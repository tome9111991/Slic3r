#ifndef SLIC3R_GUI_GL_CAMERA_HPP
#define SLIC3R_GUI_GL_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Slic3r {
namespace GUI {
namespace GL {

class Camera {
public:
    Camera();
    
    void set_viewport(int x, int y, int width, int height);
    void set_target(const glm::vec3& target);
    void rotate(float delta_theta, float delta_phi);
    void zoom(float factor);
    void pan(float dx, float dy);
    
    void set_iso_view(int dir_idx = 0); // 0=Top, 1=Bottom, etc.
    
    glm::mat4 get_view_matrix() const;
    glm::mat4 get_projection_matrix() const;
    glm::vec3 get_position() const;

    // Convert screen coordinates to ray in world space
    void screen_to_world(float winx, float winy, glm::vec3& out_origin, glm::vec3& out_dir) const;

private:
    void update_camera_vectors();

    // Camera Attributes
    glm::vec3 m_target;
    float m_distance;
    float m_theta; // Pitch (from Z axis down?)
    float m_phi;   // Yaw (around Z axis)
    
    // Viewport
    int m_viewport_x, m_viewport_y, m_viewport_w, m_viewport_h;
    
    // Projection
    float m_zoom; // Ortho scale
    float m_near_plane;
    float m_far_plane;
};

} // namespace GL
} // namespace GUI
} // namespace Slic3r

#endif
