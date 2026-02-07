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
    vec3 up = (abs(dot(zAxis, vec3(0,0,1))) > 0.99) ? vec3(1,0,0) : vec3(0,0,1);
    
    vec3 xAxis = normalize(cross(up, zAxis));
    vec3 yAxis = normalize(cross(zAxis, xAxis));
    mat3 R = mat3(xAxis, yAxis, zAxis);
    
    vec3 localPos = aPos;
    localPos.x *= width * 0.5;
    localPos.y *= height * 0.495; 
    
    float ext = width * 0.4;
    localPos.z = (aPos.z < 0.5) ? -ext : len + ext;
    
    // Transform to World Space
    vec3 worldPos = pA + R * localPos;
    
    // Transform to View Space (Camera Space)
    vec4 viewPos = view * vec4(worldPos, 1.0);
    
    // Transform Normal to View Space
    // Since R is orthogonal and we assume no scaling in view matrix, mat3(view)*R is fine
    vNormal = mat3(view) * (R * aNormal);
    vFragPos = viewPos.xyz;
    vColor = inst.color;
    
    gl_Position = projection * viewPos;
}
