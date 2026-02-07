#version 460 core
out vec4 FragColor;

in vec3 vFragPos;
in vec3 vNormal;
in vec4 vColor;

uniform int u_lit;

void main() {
    if (u_lit == 0) {
        FragColor = vColor;
        return;
    }

    // Since we are in View Space, the camera is at (0,0,0)
    vec3 N = normalize(vNormal);
    
    // Headlight setup: Light comes from the camera direction
    // A slight offset helps define the 3D shape of the tubes
    const vec3 LIGHT_DIR = normalize(vec3(0.1, 0.2, 1.0)); 
    
    // Matte lighting parameters
    // Higher ambient and diffuse to keep it bright but without "glamor" glints
    const float AMBIENT = 0.45;
    const float DIFFUSE = 0.55;

    // Diffuse component (Lambertian)
    float diff = max(dot(N, LIGHT_DIR), 0.0);
    
    // Combine for a clean, bright, matte look
    vec3 lightColor = vColor.rgb * (AMBIENT + diff * DIFFUSE);
    
    FragColor = vec4(lightColor, vColor.a);
}
