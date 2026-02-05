#include "Camera.hpp"
#include <algorithm>

namespace Slic3r {
namespace GUI {
namespace GL {

Camera::Camera() 
    : m_target(0.0f, 0.0f, 0.0f)
    , m_distance(250.0f)
    , m_theta(45.0f)
    , m_phi(45.0f)
    , m_zoom(1.0f)
    , m_near_plane(-1000.0f)
    , m_far_plane(1000.0f)
    , m_viewport_x(0)
    , m_viewport_y(0)
    , m_viewport_w(800)
    , m_viewport_h(600)
{
}

void Camera::set_viewport(int x, int y, int width, int height) {
    m_viewport_x = x;
    m_viewport_y = y;
    m_viewport_w = width;
    m_viewport_h = height;
}

void Camera::set_target(const glm::vec3& target) {
    m_target = target;
}

void Camera::rotate(float delta_theta, float delta_phi) {
    m_theta += delta_theta;
    m_phi += delta_phi;
    
    // Clamp theta to avoid gimbal lock or flipping (0 to 180 degrees)
    // In our coordinate system, Z is up. 
    // Let's assume standard spherical coords:
    // Theta: angle from Z axis. 0 = top, 180 = bottom.
    // Phi: angle in XY plane.
    
    m_theta = std::max(1.0f, std::min(179.0f, m_theta));
}

void Camera::zoom(float factor) {
    m_zoom *= factor;
    // Clamp zoom
    m_zoom = std::max(0.01f, std::min(50.0f, m_zoom));
}

void Camera::pan(float dx, float dy) {
    // To pan correctly, we need to move the target in the plane of the camera
    // This requires the Right and Up vectors of the camera
    
    // Simple ortho panning:
    // dx, dy are in screen pixels.
    // We need to convert to world units.
    
    float scale_w = (float)m_viewport_w / m_zoom;
    float scale_h = (float)m_viewport_h / m_zoom;
    
    // This is approximate, ideally we use the inverse view matrix
    // But since we are Ortho, it's linear.
    // TODO: Implement proper panning relative to camera orientation
    
    // For now, simple XY pan (won't work well when rotated)
    // m_target.x -= dx * 0.1f / m_zoom;
    // m_target.y += dy * 0.1f / m_zoom;
}

glm::mat4 Camera::get_view_matrix() const {
    // Calculate position from spherical coordinates
    float rad_theta = glm::radians(m_theta);
    float rad_phi = glm::radians(m_phi);
    
    float x = m_distance * sin(rad_theta) * cos(rad_phi);
    float y = m_distance * sin(rad_theta) * sin(rad_phi);
    float z = m_distance * cos(rad_theta);
    
    glm::vec3 pos = m_target + glm::vec3(x, y, z);
    
    // Up vector is typically Z, but if we are looking almost straight down, 
    // we must switch to Y to avoid a CrossProduct(0) which leads to NaNs.
    glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
    if (std::abs(glm::dot(glm::normalize(pos - m_target), up)) > 0.99f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    
    return glm::lookAt(pos, m_target, up);
}

glm::mat4 Camera::get_projection_matrix() const {
    float w = (float)m_viewport_w / m_zoom;
    float h = (float)m_viewport_h / m_zoom;
    
    return glm::ortho(-w/2.0f, w/2.0f, -h/2.0f, h/2.0f, m_near_plane, m_far_plane);
}

glm::vec3 Camera::get_position() const {
    float rad_theta = glm::radians(m_theta);
    float rad_phi = glm::radians(m_phi);
    float x = m_distance * sin(rad_theta) * cos(rad_phi);
    float y = m_distance * sin(rad_theta) * sin(rad_phi);
    float z = m_distance * cos(rad_theta);
    return m_target + glm::vec3(x, y, z);
}

void Camera::screen_to_world(float winx, float winy, glm::vec3& out_origin, glm::vec3& out_dir) const {
    glm::mat4 view = get_view_matrix();
    glm::mat4 proj = get_projection_matrix();
    glm::vec4 viewport(m_viewport_x, m_viewport_y, m_viewport_w, m_viewport_h);
    
    glm::vec3 win(winx, m_viewport_h - winy, 0.0f); // Invert Y
    glm::vec3 near_pt = glm::unProject(win, view, proj, viewport);
    
    win.z = 1.0f;
    glm::vec3 far_pt = glm::unProject(win, view, proj, viewport);
    
    out_origin = near_pt;
    out_dir = glm::normalize(far_pt - near_pt);
}

} // namespace GL
} // namespace GUI
} // namespace Slic3r
