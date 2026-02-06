#version 460 core
layout (location = 0) in vec3 aPos; // Template mesh vertex (0..1 Z axis)
layout (location = 1) in vec3 aNormal; // Template normal

out vec3 vFragPos;
out vec3 vNormal;
out vec4 vColor;

struct InstanceData {
    vec4 posA_width;
    vec4 posB_height;
    vec4 color;
};

// Bind binding=0 to the SSBO
layout(std430, binding = 0) buffer Instances {
    InstanceData data[];
} instances;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model; // Usually identity, but kept for compatibility

void main() {
    InstanceData inst = instances.data[gl_InstanceID];
    
    vec3 pA = inst.posA_width.xyz;
    float width = inst.posA_width.w;
    vec3 pB = inst.posB_height.xyz;
    float height = inst.posB_height.w;
    
    vec3 dir = pB - pA;
    float len = length(dir);
    
    if(len < 0.0001) {
        gl_Position = vec4(0,0,0,0);
        return;
    }
    
    vec3 zAxis = normalize(dir);
    vec3 up = vec3(0, 0, 1);
    
    // Handle case where zAxis is // to up
    if (abs(dot(zAxis, up)) > 0.99) {
        up = vec3(1, 0, 0);
    }
    
    vec3 xAxis = normalize(cross(up, zAxis));
    vec3 yAxis = normalize(cross(zAxis, xAxis));
    
    // Create rotation matrix (align Z to dir)
    mat3 R = mat3(xAxis, yAxis, zAxis);
    
    // Scale local vertex
    // x/y scaled by width/height. z scaled by length.
    vec3 localPos = aPos;
    localPos.x *= width * 0.5;
    // Scale height slightly < 1.0 to avoid Z-fighting with layers exactly above/below
    localPos.y *= height * 0.495; 
    
    // Extend start/end to create "caps" and close gaps at corners
    // Extends by 40% of width
    float ext = width * 0.4;
    if (aPos.z < 0.5) {
        localPos.z = -ext;
    } else {
        localPos.z = len + ext;
    }
    
    // Transform
    vec3 worldPos = pA + R * localPos;
    
    // Normal transform (no non-uniform scaling on normal ideally, but approximation)
    vec3 localNormal = aNormal;
    // localNormal.z is 0 for tube side
    vec3 worldNormal = R * localNormal;
    
    vFragPos = worldPos;
    vNormal = worldNormal;
    vColor = inst.color;
    
    gl_Position = projection * view * vec4(worldPos, 1.0);
}
