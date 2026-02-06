#ifndef CANVAS_THEME_HPP
#define CANVAS_THEME_HPP

#include <wx/wx.h>
#include <string>
#include "ExtrusionEntity.hpp"

namespace Slic3r { namespace GUI {

struct CanvasThemeColors {
    std::string name;
    
    // Canvas Background (3D Viewport)
    wxColour canvas_bg_top;
    wxColour canvas_bg_bottom;
    wxColour canvas_plate_bg; // 2D Plater background
    bool solid_background; // If true, use canvas_bg_top for solid fill

    // Interaction
    wxColour selected_color; // Keep for 3D selection if distinct from accent
    wxColour hover_color;

    // 3D Scene Elements
    wxColour grid_color;
    wxColour ground_color;
    wxColour cutplane_color;

    // Model Representation
    wxColour color_parts;
    wxColour color_infill;
    wxColour color_support;
    wxColour color_unknown;

    // Bed Visualization
    wxColour bed_color;
    wxColour bed_grid;
    wxColour bed_selected;
    wxColour bed_objects;
    wxColour bed_instance;
    wxColour bed_dragged;
    wxColour bed_center;
    wxColour bed_skirt;
    wxColour bed_clearance;
    wxColour bed_dark;

    // G-Code Roles (New State-of-the-Art)
    wxColour role_perimeter;
    wxColour role_external_perimeter;
    wxColour role_overhang_perimeter;
    wxColour role_internal_infill;
    wxColour role_solid_infill;
    wxColour role_top_solid_infill;
    wxColour role_bridge_infill;
    wxColour role_gap_fill;
    wxColour role_skirt;
    wxColour role_support_material;
    wxColour role_support_material_interface;

    // Splines (Layer Height Editor)
    wxColour spline_l_pen;
    wxColour spline_o_pen;
    wxColour spline_i_pen;
    wxColour spline_r_pen;

    wxColour get_role_color(ExtrusionRole role) const;
};

class CanvasTheme {
public:
    static CanvasThemeColors GetColors();
};

}} // namespace Slic3r::GUI

#endif // CANVAS_THEME_HPP
