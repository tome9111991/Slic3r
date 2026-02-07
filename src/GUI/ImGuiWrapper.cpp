#include "ImGuiWrapper.hpp"
#include <glad/glad.h> 
#include <wx/window.h>
#include <imgui_impl_opengl3.h>
#include <imgui.h>
#include "Theme/ThemeManager.hpp"

namespace Slic3r {
namespace GUI {

ImGuiWrapper::ImGuiWrapper()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    // Initialize OpenGL3 adapter
    // We assume GL context is already initialized by wxWidgets before this is called
    ImGui_ImplOpenGL3_Init("#version 460"); 
}

ImGuiWrapper::~ImGuiWrapper()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiWrapper::new_frame(int width, int height)
{
    if (m_new_frame_open) return;

    ImGui_ImplOpenGL3_NewFrame();
    
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);

    ImGui::NewFrame();
    m_new_frame_open = true;
}

void ImGuiWrapper::render()
{
    if (!m_new_frame_open) return;

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    m_new_frame_open = false;
}

bool ImGuiWrapper::update_mouse_data(wxMouseEvent& evt)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)evt.GetX(), (float)evt.GetY());
    io.MouseDown[0] = evt.LeftIsDown();
    io.MouseDown[1] = evt.RightIsDown();
    io.MouseDown[2] = evt.MiddleIsDown();
    
    float wheel = (float)evt.GetWheelRotation() / (float)evt.GetWheelDelta();
    if (wheel != 0) io.MouseWheel += wheel;

    return io.WantCaptureMouse;
}

bool ImGuiWrapper::update_key_data(wxKeyEvent& evt)
{
    // TODO: Implement key mapping for ImGui 1.87+ using io.AddKeyEvent()
    // The explicit KeysDown array and KeyCtrl bools are removed in newer versions.
    // For now, we disable keyboard input to allow compilation.
    
    // int key = evt.GetKeyCode();
    // if (key >= 0 && key < 512) { ... }
    
    return false; // io.WantCaptureKeyboard; 
}

void ImGuiWrapper::set_scaling(float scale)
{
    m_scale = scale;
    ImGui::GetStyle().ScaleAllSizes(scale);
}

void ImGuiWrapper::init_style()
{
    // Custom style can go here
}

unsigned int ImGuiWrapper::load_texture(const std::string& name)
{
    // 1. Get Bitmap from ThemeManager (Assuming 24x24 for toolbar buttons)
    // Note: ThemeManager::GetSVG expects wxString
    wxBitmapBundle bundle = ThemeManager::GetSVG(wxString::FromUTF8(name.c_str()), wxSize(24, 24));
    
    if (!bundle.IsOk()) return 0;
    
    // Convert to wxImage to access raw data
    wxBitmap bmp = bundle.GetBitmap(wxSize(24, 24));
    wxImage img = bmp.ConvertToImage();
    
    if (!img.IsOk()) return 0;

    int w = img.GetWidth();
    int h = img.GetHeight();
    
    // Convert to RGBA buffer
    unsigned char* data = (unsigned char*)malloc(w * h * 4);
    unsigned char* src_rgb = img.GetData();
    unsigned char* src_alpha = img.GetAlpha();
    
    for (int i = 0; i < w * h; i++) {
        data[i*4 + 0] = src_rgb[i*3 + 0];
        data[i*4 + 1] = src_rgb[i*3 + 1];
        data[i*4 + 2] = src_rgb[i*3 + 2];
        if (src_alpha) {
            data[i*4 + 3] = src_alpha[i];
        } else {
             data[i*4 + 3] = 255;
        }
    }

    // Upload to OpenGL
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    free(data);
    
    return texture_id;
}

} // namespace GUI
} // namespace Slic3r
