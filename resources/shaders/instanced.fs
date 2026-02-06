#version 460 core
out vec4 FragColor;

in vec3 vFragPos;
in vec3 vNormal;
in vec4 vColor;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform int u_lit;
uniform float u_clipping_z; // Clipping height (World Z)

void main() {
    // Clipping (Moved to CPU: we just don't draw the instances!)
    // if (vFragPos.z > u_clipping_z + 0.005) discard;

    if (u_lit == 0) {
        FragColor = vColor;
        return;
    }

    // Simple lighting
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(vec3(0.5, 0.5, 1.0)); // Fixed light direction
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.2); // 0.2 ambient
    vec3 diffuse = diff * vColor.rgb;
    
    FragColor = vec4(diffuse, vColor.a);
}
