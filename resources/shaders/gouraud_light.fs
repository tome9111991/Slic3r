#version 460 core
out vec4 FragColor;

in vec3 v_pos_view;
in vec3 v_normal_view;
in float v_z_height;
in float v_tube;

uniform vec3 objectColor;
uniform float u_clipping_z;
uniform float u_alpha;
uniform int u_lit; // 1 = lit, 0 = flat

// PrusaSlicer Lighting Constants
const vec3 LIGHT1_DIR = normalize(vec3(-0.6, 0.6, 1.0)); // Top-Left
const vec3 LIGHT2_DIR = normalize(vec3(1.0, 0.2, 1.0));  // Front-Right

const float INTENSITY_CORRECTION = 0.6;
const float INTENSITY_AMBIENT = 0.3;
const float LIGHT1_DIFFUSE = 0.8 * INTENSITY_CORRECTION;
const float LIGHT1_SPECULAR = 0.125 * INTENSITY_CORRECTION;
const float LIGHT1_SHININESS = 20.0;
const float LIGHT2_DIFFUSE = 0.3 * INTENSITY_CORRECTION;

void main() {
    if (v_z_height > u_clipping_z) discard;
    
    if (u_lit == 0) {
        FragColor = vec4(objectColor, u_alpha);
        return;
    }

    vec3 N = normalize(v_normal_view);
    vec3 V = normalize(-v_pos_view); // View direction is towards origin (0,0,0) in View Space
    
    // Light 1 (Top)
    float diff1 = max(dot(N, LIGHT1_DIR), 0.0);
    
    // Blinn-Phong Specular for Light 1
    vec3 H1 = normalize(LIGHT1_DIR + V);
    float spec1 = LIGHT1_SPECULAR * pow(max(dot(N, H1), 0.0), LIGHT1_SHININESS);
    
    // Light 2 (Front) - Diffuse Only
    float diff2 = max(dot(N, LIGHT2_DIR), 0.0);
    
    // Combine
    float intensity = INTENSITY_AMBIENT + (diff1 * LIGHT1_DIFFUSE) + (diff2 * LIGHT2_DIFFUSE) + spec1;
    
    vec3 finalColor = objectColor * intensity;
    FragColor = vec4(finalColor, u_alpha);
}
