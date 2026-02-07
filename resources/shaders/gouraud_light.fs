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

// Headlight Setup (Fixed in View Space)
const vec3 LIGHT1_DIR = normalize(vec3(0.1, 0.2, 1.0)); // Main headlight, slightly offset
const vec3 LIGHT2_DIR = normalize(vec3(-0.5, 0.5, 0.5)); // Side fill light

const float INTENSITY_AMBIENT = 0.35;
const float LIGHT1_DIFFUSE = 0.75;
const float LIGHT1_SPECULAR = 0.3;
const float LIGHT1_SHININESS = 32.0;

const float LIGHT2_DIFFUSE = 0.2;

void main() {
    if (v_z_height > u_clipping_z) discard;
    
    if (u_lit == 0) {
        FragColor = vec4(objectColor, u_alpha);
        return;
    }

    vec3 N = normalize(v_normal_view);
    vec3 V = normalize(-v_pos_view); 
    
    // Light 1 (Headlight)
    float diff1 = max(dot(N, LIGHT1_DIR), 0.0);
    
    // Blinn-Phong Specular
    vec3 H1 = normalize(LIGHT1_DIR + V);
    float spec1 = pow(max(dot(N, H1), 0.0), LIGHT1_SHININESS) * LIGHT1_SPECULAR;
    
    // Light 2 (Fill)
    float diff2 = max(dot(N, LIGHT2_DIR), 0.0);
    
    // Combine
    float intensity = INTENSITY_AMBIENT + (diff1 * LIGHT1_DIFFUSE) + (diff2 * LIGHT2_DIFFUSE);
    
    vec3 finalColor = objectColor * intensity + vec3(spec1);
    FragColor = vec4(finalColor, u_alpha);
}
