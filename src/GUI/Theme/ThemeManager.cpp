#include "ThemeManager.hpp"
#include <wx/stdpaths.h>
#include <wx/filename.h>

namespace Slic3r { namespace GUI {

bool ThemeManager::m_isDark = false;

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

wxBitmapBundle ThemeManager::GetSVG(const wxString& iconName, const wxSize& size) {
    // Determine the base path for resources
    // In a real deployment this might need to be smarter (e.g. referencing an installed share dir)
    // For now, we assume "resources/icons/" is relative to the working directory or executable
    
    // Attempt to locate resources relative to the executable
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName fname(exePath);
    wxString appDir = fname.GetPath();

    // Check standard locations (dev vs installed)
    wxString resourceBase = appDir + "/../resources/icons/";
    if (!wxDirExists(resourceBase)) {
        resourceBase = "resources/icons/"; // Fallback to current working directory
    }
    
    wxString themeSubDir = m_isDark ? "dark/" : "light/";
    wxString fullPath = resourceBase + themeSubDir + iconName + ".svg";

    if (!wxFileExists(fullPath)) {
        // Fallback or warning? For now just return empty bundle or try to load anyway (wx might handle it)
        // If file doesn't exist, Create might fail gracefully or return empty bitmap
    }

    return wxBitmapBundle::FromSVGFile(fullPath, size);
}

}} // namespace Slic3r::GUI
