#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in float aTube;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

out vec3 v_pos_view;
out vec3 v_normal_view;
out float v_z_height;
out float v_tube;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    vec4 viewPos = view * worldPos;
    
    v_pos_view = viewPos.xyz;
    // Transform normal to View Space (correctly handling non-uniform scaling)
    v_normal_view = normalize(mat3(transpose(inverse(view * model))) * aNormal);
    
    v_z_height = aPos.z;
    v_tube = aTube;
    gl_Position = projection * viewPos;
}
