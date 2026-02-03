#include "ThemeManager.hpp"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/sstream.h>

namespace Slic3r { namespace GUI {

bool ThemeManager::m_isDark = true;

void ThemeManager::SetDarkMode(bool dark) {
    m_isDark = dark;
}

bool ThemeManager::IsDark() {
    return m_isDark;
}

ThemeColors ThemeManager::GetColors() {
    if (m_isDark) {
        return { 
            wxColour(31, 31, 31),    // bg (Main Content)
            wxColour(45, 45, 45),    // surface (Panels/Inputs)
            wxColour(240, 240, 240), // text
            wxColour(150, 150, 150), // textMuted
            wxColour(0, 174, 126),   // accent (Orca Green)
            wxColour(70, 70, 70),    // border
            wxColour(24, 24, 24),    // header (Top Bar)
            true                     // isDark
        };
    }
    return { 
        wxColour(240, 240, 240), // bg
        wxColour(255, 255, 255), // surface
        wxColour(30, 30, 30),    // text
        wxColour(100, 100, 100), // textMuted
        wxColour(0, 90, 180),    // accent
        wxColour(200, 200, 200), // border
        wxColour(230, 230, 230), // header
        false                    // isDark
    };
}

wxBitmapBundle ThemeManager::GetSVG(const wxString& iconName, const wxSize& size, const wxColour& color) {
    // Attempt to locate resources relative to the executable
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName fname(exePath);
    wxString appDir = fname.GetPath();

    // Check standard locations (dev vs installed)
    wxString resourceBase = appDir + "/../resources/images/";
    if (!wxDirExists(resourceBase)) {
        resourceBase = "resources/images/"; // Fallback to current working directory
    }
    
    wxString fullPath = resourceBase + iconName + ".svg";

    if (!wxFileExists(fullPath)) {
        // Fallback: Try with 'icons' folder just in case
        fullPath = resourceBase + "../icons/" + (m_isDark ? "dark/" : "light/") + iconName + ".svg";
        if (!wxFileExists(fullPath)) return wxBitmapBundle();
    }

    // Read SVG content
    wxFileInputStream input(fullPath);
    if (!input.IsOk()) return wxBitmapBundle();
    
    wxString svgContent;
    wxStringOutputStream output(&svgContent);
    input.Read(output);

    // Recolor if requested
    if (color.IsOk()) {
        wxString hexColor = color.GetAsString(wxC2S_HTML_SYNTAX);
        // Replace standard placeholders or common colors
        // Ideally SVGs should use a specific placeholder like #PLACEHOLDER
        // But for 'tick.svg' it uses #333.
        svgContent.Replace("#333", hexColor); 
        svgContent.Replace("#000000", hexColor);
        svgContent.Replace("#000", hexColor);
        svgContent.Replace("fill=\"black\"", "fill=\"" + hexColor + "\"");
        svgContent.Replace("stroke=\"black\"", "stroke=\"" + hexColor + "\"");
        svgContent.Replace("stroke=\"currentColor\"", "stroke=\"" + hexColor + "\"");
        svgContent.Replace("fill=\"currentColor\"", "fill=\"" + hexColor + "\"");
    }

    return wxBitmapBundle::FromSVG(svgContent.ToStdString().c_str(), size);
}

}} // namespace Slic3r::GUI
