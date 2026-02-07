#ifndef slic3r_ImGuiWrapper_hpp_
#define slic3r_ImGuiWrapper_hpp_

#include <imgui.h>
#include <wx/event.h>
#include <string>

namespace Slic3r {
namespace GUI {

class ImGuiWrapper
{
public:
    ImGuiWrapper();
    ~ImGuiWrapper();

    // Initialization and Frame Management
    void new_frame(int width, int height);
    void render();
    
    // Event Forwarding (return true if ImGui handled the event)
    bool update_mouse_data(wxMouseEvent& evt);
    bool update_key_data(wxKeyEvent& evt);

    // Scaling
    void set_scaling(float scale);
    
    // Texture Management
    unsigned int load_texture(const std::string& name);

private:
    void init_style();
    bool m_new_frame_open { false };
    float m_scale { 1.0f };
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_ImGuiWrapper_hpp_
