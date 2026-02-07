#include "ImGuiToolbar.hpp"
#include <imgui.h>
#include <map>
#include <string>
#include <cstdint>
#include "GUI/Theme/ThemeManager.hpp"
#include <glad/glad.h>

namespace Slic3r {
namespace GUI {

// Internal helper class to manage textures, similar to OrcaSlicer's approach but using wxWidgets for SVG rasterization
class ImGuiTextureCache {
public:
    static unsigned int GetTexture(const std::string& name, const wxSize& size, const wxColour& color) {
        static ImGuiTextureCache instance;
        return instance.load_icon(name, size, color);
    }

private:
    struct CacheKey {
        std::string name;
        int w, h;
        unsigned int color;

        bool operator<(const CacheKey& other) const {
            if (name != other.name) return name < other.name;
            if (w != other.w) return w < other.w;
            if (h != other.h) return h < other.h;
            return color < other.color;
        }
    };

    std::map<CacheKey, unsigned int> m_cache;

    unsigned int load_icon(const std::string& name, const wxSize& size, const wxColour& color) {
        CacheKey key{name, size.x, size.y, color.GetRGBA()};
        if (m_cache.find(key) != m_cache.end()) 
            return m_cache[key];

        // Ensure we find the icon even if extension is missing or different
        wxBitmapBundle bundle = ThemeManager::GetSVG(wxString::FromUTF8(name.c_str()), size, color);
        if (!bundle.IsOk()) return 0;
        
        wxBitmap bmp = bundle.GetBitmap(size);
        wxImage img = bmp.ConvertToImage();
        if (!img.IsOk()) return 0;

        int w = img.GetWidth();
        int h = img.GetHeight();
        unsigned char* data = (unsigned char*)malloc(w * h * 4);
        unsigned char* src_rgb = img.GetData();
        unsigned char* src_alpha = img.GetAlpha();
        
        for (int i = 0; i < w * h; i++) {
            data[i*4 + 0] = src_rgb[i*3 + 0];
            data[i*4 + 1] = src_rgb[i*3 + 1];
            data[i*4 + 2] = src_rgb[i*3 + 2];
            // SVG usually has alpha, but if not (e.g. BMP converted), assume opaque
            data[i*4 + 3] = src_alpha ? src_alpha[i] : 255;
        }

        GLuint texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        // Bilinear filtering for smooth scaling if needed
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        
        glBindTexture(GL_TEXTURE_2D, 0);
        free(data);
        
        m_cache[key] = texture_id;
        return texture_id;
    }
};

void ImGuiToolbar::draw(const PlaterActions& actions, float canvas_width, float canvas_height) {
    
    // Config
    ThemeColors theme = ThemeManager::GetColors();
    float padding = 0.0f;
    ImVec2 btn_size(36, 36);
    wxSize svg_size(36, 36);
    
    // Position: Top-Center (No top gap)
    ImGui::SetNextWindowPos(ImVec2(canvas_width * 0.5f, padding), ImGuiCond_Always, ImVec2(0.5f, 0.0f)); 
    
    // Styling
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    
    // Background Color from Theme with 75% transparency (works across all themes)
    ImVec4 bg_col(theme.header.Red() / 255.0f, theme.header.Green() / 255.0f, theme.header.Blue() / 255.0f, 0.75f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_col);
    
    // Border Color from Theme
    ImVec4 border_col(theme.border.Red() / 255.0f, theme.border.Green() / 255.0f, theme.border.Blue() / 255.0f, 0.8f);
    ImGui::PushStyleColor(ImGuiCol_Border, border_col);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    
    // Flags: NoScrollbar added for extra safety
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | 
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | 
                             ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;

    if (ImGui::Begin("Tools", nullptr, flags)) {
        
        auto btn = [&](const char* icon, const char* tooltip, std::function<void()> cb) {
            ImGui::PushID(icon);
            unsigned int tex = ImGuiTextureCache::GetTexture(icon, svg_size, theme.text);
            
            bool clicked = false;
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.15f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,1,1,0.25f));

            // Use an invisible-ish button for hits, then draw image inside
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            std::string btn_id = std::string("##") + icon;
            clicked = ImGui::Button(btn_id.c_str(), btn_size);
            
            if (tex != 0) {
                // Draw the image on top of the button
                ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)tex, 
                    ImVec2(p0.x + 4, p0.y + 4), 
                    ImVec2(p0.x + btn_size.x - 4, p0.y + btn_size.y - 4));
            } else {
                 ImGui::GetWindowDrawList()->AddText(ImVec2(p0.x + 12, p0.y + 10), ImColor(255,255,255), "?");
            }
            
            ImGui::PopStyleColor(3);

            if (clicked && cb) {
                cb();
            }

            if (ImGui::IsItemHovered() && tooltip) {
                ImGui::SetTooltip("%s", tooltip);
            }
            ImGui::PopID();
        };
        
        // Separator Helper
        auto sep = [&]() {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(theme.textMuted.Red()/255.0f, theme.textMuted.Green()/255.0f, theme.textMuted.Blue()/255.0f, 0.3f), "|");
            ImGui::SameLine();
        };

        // --- Toolbar Layout (Dynamic) ---
        bool first = true;
        for (const auto& item : actions) {
            if (item.is_separator) {
                sep();
            } else {
                if (!first) ImGui::SameLine();
                btn(item.icon.c_str(), item.tooltip.c_str(), item.callback);
            }
            first = false;
        }
        
    }
    ImGui::End();
    
    ImGui::PopStyleColor(2); // WindowBg, Border
    ImGui::PopStyleVar(5); // WindowRounding, WindowPadding, ItemSpacing, FrameRounding, WindowBorderSize
}

void* ImGuiToolbar::get_texture(const std::string& icon_name) {
    ThemeColors theme = ThemeManager::GetColors();
    return (void*)(intptr_t)ImGuiTextureCache::GetTexture(icon_name, wxSize(36, 36), theme.text);
}

} // namespace GUI
} // namespace Slic3r
