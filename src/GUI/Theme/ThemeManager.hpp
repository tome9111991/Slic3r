#ifndef THEMEMANAGER_HPP
#define THEMEMANAGER_HPP

#include <wx/wx.h>
#include <wx/bmpbndl.h>
#include <wx/graphics.h>
#include <string>

namespace Slic3r { namespace GUI {

struct ThemeColors {
    wxColour bg;          // Window/Control background
    wxColour surface;     // Button/Input background
    wxColour text;        // Main text
    wxColour textMuted;   // Gray text / Disabled
    wxColour accent;      // Highlight (e.g. Blue)
    wxColour border;      // Border color
    wxColour header;      // Top bar / Toolbar background
    bool isDark;
};

class ThemeManager {
public:
    static void SetDarkMode(bool dark);
    static bool IsDark();

    static ThemeColors GetColors();

    // Automatically loads the correct SVG depending on the theme
    // Expected path: resources/icons/light/name.svg or resources/icons/dark/name.svg
    static wxBitmapBundle GetSVG(const wxString& iconName, const wxSize& size);

private:
    static bool m_isDark;
};

}} // namespace Slic3r::GUI

#endif // THEMEMANAGER_HPP
