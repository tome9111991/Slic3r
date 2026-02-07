#ifndef slic3r_GUI_ImGuiToolbar_hpp_
#define slic3r_GUI_ImGuiToolbar_hpp_

#include <functional>
#include <string>

namespace Slic3r {
namespace GUI {

struct ToolbarItem {
    std::string icon;
    std::string tooltip;
    std::function<void()> callback;
    bool is_separator = false;
    
    // Helper factories
    static ToolbarItem Action(const std::string& icon, const std::string& tooltip, std::function<void()> cb) {
        return {icon, tooltip, cb, false};
    }
    static ToolbarItem Separator() {
        return {"", "", nullptr, true};
    }
};

using PlaterActions = std::vector<ToolbarItem>;

class ImGuiToolbar {
public:
    static void draw(const PlaterActions& actions, float canvas_width, float canvas_height);
    
    // Helper to get texture (lazy load)
    static void* get_texture(const std::string& icon_name);
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_GUI_ImGuiToolbar_hpp_
