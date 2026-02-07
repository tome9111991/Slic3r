#ifndef THEMEMANAGER_HPP
#define THEMEMANAGER_HPP

#include <wx/wx.h>
#include <wx/bmpbndl.h>
#include <wx/graphics.h>
#include <map>
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

    // Trigger a global refresh of the UI
    static void UpdateUI();
    
    // Apply styling to a window and its children recursively
    static void ApplyThemeRecursive(wxWindow* root);

    // Automatically loads the correct SVG depending on the theme
    // Expected path: resources/images/name.svg
    // If color is provided, attempts to recolor the SVG stroke/fill
    static wxBitmapBundle GetSVG(const wxString& iconName, const wxSize& size, const wxColour& color = wxNullColour);

    // Font Management
    enum class FontSize {
        Small,  // Standard UI elements
        Medium, // Headers, Important labels (approx 12pt)
        Large   // Section headers
    };

    enum class FontWeight {
        Normal,
        Bold
    };

    static wxFont GetFont(FontSize size = FontSize::Small, FontWeight weight = FontWeight::Normal);


private:
    static bool m_isDark;
    
    // Cache Key Structure
    struct CacheKey {
        wxString name;
        int w, h;
        unsigned long color; // RGBA as long for simple key

        bool operator<(const CacheKey& other) const {
             if (name != other.name) return name < other.name;
             if (w != other.w) return w < other.w;
             if (h != other.h) return h < other.h;
             return color < other.color;
        }
    };
    
    static std::map<CacheKey, wxBitmapBundle> m_iconCache;
    static void ClearIconCache();
};

}} // namespace Slic3r::GUI

#endif // THEMEMANAGER_HPP
