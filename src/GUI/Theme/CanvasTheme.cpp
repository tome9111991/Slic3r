#include "CanvasTheme.hpp"
#include "ThemeManager.hpp"

namespace Slic3r { namespace GUI {

CanvasThemeColors CanvasTheme::GetColors() {
    bool dark = ThemeManager::IsDark();

    CanvasThemeColors c;

    if (dark) {
        c.name = "Dark";
        
        c.solid_background = true;
        c.canvas_bg_top = wxColour(31, 31, 31);
        c.canvas_bg_bottom = wxColour(31, 31, 31);
        c.canvas_plate_bg = wxColour(31, 31, 31);

        // Dark Mode Colors (from old Dark.hpp)
        c.selected_color = wxColour(0, 174, 126); // Orca Green
        c.hover_color    = wxColour(0, 214, 155); 

        c.grid_color   = wxColour(60, 60, 60, 100);
        c.ground_color = wxColour(45, 45, 45, 100);
        c.cutplane_color = wxColour(200, 200, 200, 150);

        c.color_parts   = wxColour(100, 200, 100, 50);
        c.color_infill  = wxColour(200, 100, 100);
        c.color_support = wxColour(0, 174, 126);
        c.color_unknown = wxColour(100, 100, 200);

        c.bed_color     = wxColour(40, 40, 40);
        c.bed_grid      = wxColour(70, 70, 70);
        c.bed_selected  = wxColour(0, 174, 126);
        c.bed_objects   = wxColour(100, 100, 100);
        c.bed_instance  = wxColour(200, 80, 80);
        c.bed_dragged   = wxColour(80, 80, 200);
        c.bed_center    = wxColour(255, 255, 255);
        c.bed_skirt     = wxColour(100, 100, 100);
        c.bed_clearance = wxColour(0, 0, 200);
        c.bed_dark      = wxColour(255, 255, 255);

        c.tool_dark      = wxColour(255, 255, 255); // Inverted for dark mode
        c.tool_support   = wxColour(0, 174, 126);
        c.tool_infill    = wxColour(180, 50, 50);
        c.tool_stepperim = wxColour(200, 200, 0);
        c.tool_shade     = wxColour(50, 50, 50);
        c.tool_color     = wxColour(200, 200, 200);

        c.spline_l_pen = wxColour(200, 200, 200);
        c.spline_o_pen = wxColour(100, 100, 100);
        c.spline_i_pen = wxColour(255, 0, 0);
        c.spline_r_pen = wxColour(0, 200, 200);

    } else {
        c.name = "Light";
        
        c.solid_background = false;
        c.canvas_bg_top = wxColour(10, 98, 144);
        c.canvas_bg_bottom = wxColour(0, 0, 0);
        c.canvas_plate_bg = wxColour(255, 255, 255);

        // Light Mode Colors (from old Light.hpp)
        c.selected_color = wxColour(0, 255, 0);
        c.hover_color    = wxColour(255*0.4, 255*0.9, 0);

        c.grid_color   = wxColour(255*0.2, 255*0.2, 255*0.2, 255*0.4);
        c.ground_color = wxColour(255*0.8, 255*0.6, 255*0.5, 255*0.4);
        c.cutplane_color = wxColour(255*0.8, 255*0.8, 255*0.8, 255*0.5);

        c.color_parts   = wxColour(255, 255*0.95, 255*0.2);
        c.color_infill  = wxColour(255, 255*0.45, 255*0.45);
        c.color_support = wxColour(255*0.5, 1, 255*0.5);
        c.color_unknown = wxColour(255*0.5, 255*0.5, 1);

        c.bed_color     = wxColour(255, 255, 255);
        c.bed_grid      = wxColour(230, 230, 230);
        c.bed_selected  = wxColour(255, 166, 128);
        c.bed_objects   = wxColour(210, 210, 210);
        c.bed_instance  = wxColour(255, 128, 128);
        c.bed_dragged   = wxColour(128, 128, 255);
        c.bed_center    = wxColour(200, 200, 200);
        c.bed_skirt     = wxColour(150, 150, 150);
        c.bed_clearance = wxColour(0, 0, 200);
        c.bed_dark      = wxColour(0, 0, 0);

        c.tool_dark      = wxColour(0, 0, 0);
        c.tool_support   = wxColour(0, 0, 0);
        c.tool_infill    = wxColour(0, 0, 255*0.7);
        c.tool_stepperim = wxColour(255*0.7, 0, 0);
        c.tool_shade     = wxColour(255*0.95, 255*0.95, 255*0.95);
        c.tool_color     = wxColour(255*0.9, 255*0.9, 255*0.9);

        c.spline_l_pen = wxColour(50, 50, 50);
        c.spline_o_pen = wxColour(200, 200, 200);
        c.spline_i_pen = wxColour(255, 0, 0);
        c.spline_r_pen = wxColour(5, 120, 160);
    }

    return c;
}

}} // namespace Slic3r::GUI
