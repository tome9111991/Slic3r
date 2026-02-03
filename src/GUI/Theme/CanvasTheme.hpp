#ifndef CANVAS_THEME_HPP
#define CANVAS_THEME_HPP

#include <wx/wx.h>
#include <string>

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

    // Toolpaths / GCode
    wxColour tool_dark;
    wxColour tool_support;
    wxColour tool_infill;
    wxColour tool_stepperim;
    wxColour tool_shade;
    wxColour tool_color;

    // Splines (Layer Height Editor)
    wxColour spline_l_pen;
    wxColour spline_o_pen;
    wxColour spline_i_pen;
    wxColour spline_r_pen;
};

class CanvasTheme {
public:
    static CanvasThemeColors GetColors();
};

}} // namespace Slic3r::GUI

#endif // CANVAS_THEME_HPP
