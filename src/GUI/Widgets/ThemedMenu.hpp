#ifndef SLIC3R_GUI_THEMED_MENU_HPP
#define SLIC3R_GUI_THEMED_MENU_HPP

#include <wx/wx.h>
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace Slic3r {
namespace GUI {

class ThemedMenu {
public:
    struct Item {
        int id;
        wxString label;
        wxString help;
        wxString icon;
        wxString accel;
        bool isSeparator = false;
        bool isEnabled = true;
        bool isChecked = false;
        bool isCheckable = false;
        
        // The core: the action to perform
        std::function<void(wxCommandEvent&)> action;
        
        // Submenu support
        std::shared_ptr<ThemedMenu> subMenu;

        Item() : id(wxID_ANY) {}
        
        void Check(bool check) { if (isCheckable) isChecked = check; }
    };

    ThemedMenu() {}
    ~ThemedMenu() {}

    // Compatibility API with wxMenu
    void Append(int id, const wxString& label, const wxString& help = "", wxItemKind kind = wxITEM_NORMAL);
    void AppendSeparator();
    void AppendSubMenu(ThemedMenu* submenu, const wxString& label, const wxString& help = "");

    // Slic3r specific helper (matching our append_menu_item logic)
    // Returns pointer to the created item (valid as long as menu exists and is not cleared)
    Item* AddItem(const wxString& label, const wxString& help, std::function<void(wxCommandEvent&)> action, 
                 int id = wxID_ANY, const wxString& icon = "", const wxString& accel = "", wxItemKind kind = wxITEM_NORMAL);

    const std::vector<Item>& GetItems() const { return m_items; }
    
    // API to match wxMenu usage in Plater.cpp
    void Clear();
    size_t GetMenuItemCount() const;
    Item* FindItemByPosition(size_t pos);
    
    // Execute the action for a specific ID
    void Trigger(int id);

private:
    std::vector<Item> m_items;
};

} // namespace GUI
} // namespace Slic3r

#endif // SLIC3R_GUI_THEMED_MENU_HPP