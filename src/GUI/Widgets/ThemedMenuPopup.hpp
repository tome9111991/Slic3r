#ifndef SLIC3R_GUI_THEMED_MENU_POPUP_HPP
#define SLIC3R_GUI_THEMED_MENU_POPUP_HPP

#include <wx/wx.h>
#include <wx/popupwin.h>
#include <vector>
#include <functional>
#include "ThemedMenu.hpp"

namespace Slic3r {
namespace GUI {

class ThemedMenuPopup : public wxPopupTransientWindow {
public:
    ThemedMenuPopup(wxWindow* parent, ThemedMenu* menu, ThemedMenuPopup* parentPopup = nullptr);
    virtual ~ThemedMenuPopup();

    // Override to dismiss properly
    void OnDismiss() override;

    void SetOwnsMenu(bool owns) { m_ownsMenu = owns; }

    // Process left click event, return true if handled
    bool ProcessLeftDown(wxMouseEvent& event) override;

    void SetOnDismissCallback(std::function<void()> callback) { m_onDismiss = callback; }

protected:
    void OnEraseBackground(wxEraseEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnMotion(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnLeaveWindow(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);

private:
    ThemedMenu* m_menu; 
    ThemedMenuPopup* m_parentPopup = nullptr;
    bool m_ownsMenu = false;
    int m_hoveredIndex = -1;
    
    struct ItemInfo {
        int id;
        wxString label;
        wxString icon; // Store icon filename
        wxString accel; 
        bool isSeparator;
        bool isEnabled;
        bool isChecked;
        bool hasSubMenu;
        std::shared_ptr<ThemedMenu> subMenu;
        wxRect rect;

        // Cache for bitmaps to avoid reloading from SVG in OnPaint
        wxBitmap bundleNormal;
        wxBitmap bundleHover;
    };

    std::vector<ItemInfo> m_items;
    wxFont m_font;

    // Helper to calculate layout and window size
    void CalculateLayout();

    // Cache icons for all items
    void PrecacheIcons();
    
    // Execute the action for the item
    void ExecuteItem(const ItemInfo& item);

    std::function<void()> m_onDismiss;

    wxDECLARE_EVENT_TABLE();
};

} // namespace GUI
} // namespace Slic3r

#endif // SLIC3R_GUI_THEMED_MENU_POPUP_HPP