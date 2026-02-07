#include "ImGuiWrapper.hpp"
#include <glad/glad.h> 
#include <wx/window.h>
#include <imgui_impl_opengl3.h>
#include <imgui.h>

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

bool ImGuiWrapper::update_mouse_data(wxMouseEvent &evt)
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

bool ImGuiWrapper::update_key_data(wxKeyEvent &evt)
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

} // namespace GUI
} // namespace Slic3r
