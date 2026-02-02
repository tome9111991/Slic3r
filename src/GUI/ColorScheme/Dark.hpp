#ifndef COLOR_DARK_HPP
#define COLOR_DARK_HPP

namespace Slic3r { namespace GUI {

class DarkColor : public ColorScheme {
public:
    const std::string name() const { return "Dark"; }
    const bool SOLID_BACKGROUNDCOLOR() const { return true; };
    
    // OrcaSlicer Green Accents
    const wxColour SELECTED_COLOR() const { return wxColour(0, 174, 126); }; // Orca Green
    const wxColour HOVER_COLOR() const { return wxColour(0, 214, 155); };    // Lighter Green
    
    // Dark Backgrounds
    const wxColour TOP_COLOR() const { return wxColour(24, 24, 24); };       // Darker Grey for Top Bar
    const wxColour BOTTOM_COLOR() const { return wxColour(24, 24, 24); };    // Darker Grey for Status Bar
    const wxColour BACKGROUND_COLOR() const { return wxColour(31, 31, 31); } // Solid Dark Content
    
    // Grid & Bed
    const wxColour GRID_COLOR() const { return wxColour(60, 60, 60, 100); }; // Subtle grid
    const wxColour GROUND_COLOR() const { return wxColour(45, 45, 45, 100); }; // Darker ground
    
    const wxColour COLOR_CUTPLANE() const { return wxColour(200, 200, 200, 150); };
    const wxColour COLOR_PARTS() const { return wxColour(100, 200, 100, 50); }; 
    const wxColour COLOR_INFILL() const { return wxColour(200, 100, 100); };
    const wxColour COLOR_SUPPORT() const { return wxColour(0, 174, 126); }; // Match accent
    const wxColour COLOR_UNKNOWN() const { return wxColour(100, 100, 200); };
    
    // Bed Plate - Darker Industrial Look
    const wxColour BED_COLOR() const { return wxColour(40, 40, 40); };
    const wxColour BED_GRID() const { return wxColour(70, 70, 70); };
    const wxColour BED_SELECTED() const { return wxColour(0, 174, 126); };
    const wxColour BED_OBJECTS() const { return wxColour(100, 100, 100); };
    const wxColour BED_INSTANCE() const { return wxColour(200, 80, 80); };
    const wxColour BED_DRAGGED() const { return wxColour(80, 80, 200); };
    const wxColour BED_CENTER() const { return wxColour(255, 255, 255); };
    const wxColour BED_SKIRT() const { return wxColour(100, 100, 100); };
    const wxColour BED_CLEARANCE() const { return wxColour(0, 0, 200); };
    
    const wxColour BACKGROUND255() const { return wxColour(31, 31, 31); };
    
    // Toolpaths
    const wxColour TOOL_DARK() const { return wxColour(255, 255, 255); }; // Inverted for dark mode
    const wxColour TOOL_SUPPORT() const { return wxColour(0, 174, 126); };
    const wxColour TOOL_INFILL() const { return wxColour(180, 50, 50); };
    const wxColour TOOL_STEPPERIM() const { return wxColour(200, 200, 0); };
    const wxColour TOOL_SHADE() const { return wxColour(50, 50, 50); };
    const wxColour TOOL_COLOR() const { return wxColour(200, 200, 200); };
    
    // Splines
    const wxColour SPLINE_L_PEN() const { return wxColour(200, 200, 200); };
    const wxColour SPLINE_O_PEN() const { return wxColour(100, 100, 100); };
    const wxColour SPLINE_I_PEN() const { return wxColour(255, 0, 0); };
    const wxColour SPLINE_R_PEN() const { return wxColour(0, 200, 200); };
    
    const wxColour BED_DARK() const { return wxColour(255, 255, 255); };
};

}} // namespace Slic3r::GUI

#endif
