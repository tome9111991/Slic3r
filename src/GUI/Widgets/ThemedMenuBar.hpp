#ifndef SLIC3R_GUI_THEMED_MENUBAR_HPP
#define SLIC3R_GUI_THEMED_MENUBAR_HPP

#include <wx/wx.h>
#include <vector>
#include <utility>
#include "ThemedMenu.hpp"

namespace Slic3r {
namespace GUI {

class ThemedMenuBar : public wxPanel {
public:
    ThemedMenuBar(wxWindow* parent);
    virtual ~ThemedMenuBar();

    // Emulate wxMenuBar::Append but with our custom ThemedMenu
    bool Append(ThemedMenu* menu, const wxString& title);
    
    // Clear all menus (deletes them) so we can rebuild
    void Clear();

    void UpdateTheme();

protected:
    void OnPaint(wxPaintEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);
    void OnLeaveWindow(wxMouseEvent& event);

private:
    struct MenuEntry {
        ThemedMenu* menu;
        wxString title;
        wxRect rect; // Calculated during paint/layout
    };

    std::vector<MenuEntry> m_menus;
    int m_hoveredIndex = -1;
    int m_openMenuIndex = -1;
    
    // Non-modal tracking
    class ThemedMenuPopup* m_currentPopup = nullptr; 

    // Helper to calculate size
    wxSize DoGetBestSize() const override;

    void OpenMenu(int index);

    wxDECLARE_EVENT_TABLE();
};

} // namespace GUI
} // namespace Slic3r

#endif // SLIC3R_GUI_THEMED_MENUBAR_HPP
