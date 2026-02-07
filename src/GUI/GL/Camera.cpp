#include "Camera.hpp"
#include <algorithm>

namespace Slic3r {
namespace GUI {
namespace GL {

Camera::Camera() 
    : m_target(0.0f, 0.0f, 0.0f)
    , m_distance(200.0f)
    , m_theta(45.0f)
    , m_phi(270.0f)
    , m_zoom(4.0f)
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
    // Clamp zoom to prevent going too far out or too close
    m_zoom = std::max(2.0f, std::min(1000.0f, m_zoom));
}

void Camera::pan(float dx, float dy) {
    if (m_zoom <= 0.0f) return;

    glm::mat4 view = get_view_matrix();
    
    // The rows of the rotation part of the view matrix are the camera's basis vectors in world space.
    // Row 0: Right vector, Row 1: Up vector
    glm::vec3 right(view[0][0], view[1][0], view[2][0]);
    glm::vec3 up(view[0][1], view[1][1], view[2][1]);
    
    // dx and dy are in pixels. m_zoom is pixels per world unit.
    // We move the target (and consequently the camera position) in the opposite direction 
    // of the mouse movement to achieve a "drag world" effect.
    float world_dx = dx / m_zoom;
    float world_dy = dy / m_zoom;
    
    m_target -= right * world_dx;
    m_target += up * world_dy; // dy is positive down in screen space, up is positive up in world space
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
