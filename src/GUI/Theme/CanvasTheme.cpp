#include "CanvasTheme.hpp"
#include "ThemeManager.hpp"

namespace Slic3r { namespace GUI {

CanvasThemeColors CanvasTheme::GetColors() {
    bool dark = ThemeManager::IsDark();

    CanvasThemeColors c;

    if (dark) {
        c.name = "Dark";
        
        c.solid_background = true;
        c.canvas_bg_top = wxColour(84, 84, 84); // Testing: Bright red
        c.canvas_bg_bottom = c.canvas_bg_top;
        c.canvas_plate_bg = wxColour(25, 25, 25);

        // Dark Mode Colors (from old Dark.hpp)
        c.selected_color = wxColour(117, 117, 12); 
        c.hover_color    = wxColour(140, 140, 14); 

        c.grid_color   = wxColour(200, 200, 200, 60);
        c.ground_color = wxColour(80, 80, 90, 180);
        c.cutplane_color = wxColour(117, 117, 12, 120);

        c.color_parts   = wxColour(100, 200, 100, 50);
        c.color_infill  = wxColour(200, 100, 100);
        c.color_support = wxColour(117, 117, 12);
        c.color_unknown = wxColour(100, 100, 200);

        c.bed_color     = wxColour(40, 40, 40);
        c.bed_grid      = wxColour(70, 70, 70);
        c.bed_selected  = wxColour(117, 117, 12);
        c.bed_objects   = wxColour(100, 100, 100);
        c.bed_instance  = wxColour(200, 80, 80);
        c.bed_dragged   = wxColour(80, 80, 200);
        c.bed_center    = wxColour(255, 255, 255);
        c.bed_skirt     = wxColour(100, 100, 100);
        c.bed_clearance = wxColour(0, 0, 200);
        c.bed_dark      = wxColour(255, 255, 255);

        // G-Code Roles (Always consistent)
        c.role_perimeter                  = wxColour(255, 255, 85);   // Yellow
        c.role_external_perimeter         = wxColour(255, 111, 0);    // Orange
        c.role_overhang_perimeter         = wxColour(6, 0, 245);      // Blue
        c.role_internal_infill            = wxColour(136, 25, 25);    // Dark Red
        c.role_solid_infill               = wxColour(162, 114, 255);  // Purple
        c.role_top_solid_infill           = wxColour(255, 20, 20);    // Red
        c.role_bridge_infill              = wxColour(107, 185, 242);  // Light Blue
        c.role_gap_fill                   = wxColour(255, 255, 255);  // White
        c.role_skirt                      = wxColour(0, 150, 0);      // Green
        c.role_support_material           = wxColour(0, 200, 0);      // Green
        c.role_support_material_interface = wxColour(0, 50, 0);       // Dark Green

        c.spline_l_pen = wxColour(200, 200, 200);
        c.spline_o_pen = wxColour(100, 100, 100);
        c.spline_i_pen = wxColour(255, 0, 0);
        c.spline_r_pen = wxColour(0, 200, 200);

    } else {
        c.name = "Light";
        
        c.solid_background = true;
        c.canvas_bg_top = wxColour(10, 98, 144); // Slic3r Blue
        c.canvas_bg_bottom = c.canvas_bg_top;
        c.canvas_plate_bg = wxColour(255, 255, 255);

        // Light Mode Colors (from old Light.hpp)
        c.selected_color = wxColour(0, 255, 0);
        c.hover_color    = wxColour(255*0.4, 255*0.9, 0);

        c.grid_color   = wxColour(51, 51, 51, 102);    // Original (0.2, 0.4 Alpha)
        c.ground_color = wxColour(204, 153, 127, 102); // Original Tan (0.8, 0.6, 0.5, 0.4 Alpha)
        c.cutplane_color = wxColour(204, 204, 204, 127);

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

        // G-Code Roles (Always consistent)
        c.role_perimeter                  = wxColour(255, 255, 85);
        c.role_external_perimeter         = wxColour(255, 111, 0);
        c.role_overhang_perimeter         = wxColour(6, 0, 245);
        c.role_internal_infill            = wxColour(136, 25, 25);
        c.role_solid_infill               = wxColour(162, 114, 255);
        c.role_top_solid_infill           = wxColour(255, 20, 20);
        c.role_bridge_infill              = wxColour(107, 185, 242);
        c.role_gap_fill                   = wxColour(255, 255, 255);
        c.role_skirt                      = wxColour(0, 150, 0);
        c.role_support_material           = wxColour(0, 200, 0);
        c.role_support_material_interface = wxColour(0, 50, 0);

        c.spline_l_pen = wxColour(50, 50, 50);
        c.spline_o_pen = wxColour(200, 200, 200);
        c.spline_i_pen = wxColour(255, 0, 0);
        c.spline_r_pen = wxColour(5, 120, 160);
    }
    return c;
}

wxColour CanvasThemeColors::get_role_color(ExtrusionRole role) const {
    switch (role) {
    case erPerimeter:               return this->role_perimeter;
    case erExternalPerimeter:       return this->role_external_perimeter;
    case erOverhangPerimeter:       return this->role_overhang_perimeter;
    case erInternalInfill:          return this->role_internal_infill;
    case erSolidInfill:             return this->role_solid_infill;
    case erTopSolidInfill:          return this->role_top_solid_infill;
    case erBridgeInfill:            return this->role_bridge_infill;
    case erGapFill:                 return this->role_gap_fill;
    case erSkirt:                   return this->role_skirt;
    case erSupportMaterial:         return this->role_support_material;
    case erSupportMaterialInterface: return this->role_support_material_interface;
    default:                        return wxColour(128, 128, 128); // Grey
    }
}

}} // namespace Slic3r::GUI
