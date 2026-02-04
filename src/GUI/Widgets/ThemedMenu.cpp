#include "ThemedMenu.hpp"

namespace Slic3r {
namespace GUI {

void ThemedMenu::Append(int id, const wxString& label, const wxString& help, wxItemKind kind)
{
    Item item;
    item.id = (id == wxID_ANY) ? wxNewId() : id;
    item.label = label;
    item.help = help;
    item.isCheckable = (kind == wxITEM_CHECK);
    m_items.push_back(item);
}

void ThemedMenu::AppendSeparator()
{
    Item item;
    item.isSeparator = true;
    m_items.push_back(item);
}

ThemedMenu::Item* ThemedMenu::AppendSubMenu(ThemedMenu* submenu, const wxString& label, const wxString& help)
{
    Item item;
    item.label = label;
    item.help = help;
    item.subMenu = std::shared_ptr<ThemedMenu>(submenu); // Take ownership or share
    m_items.push_back(item);
    return &m_items.back();
}

ThemedMenu::Item* ThemedMenu::AddItem(const wxString& label, const wxString& help, std::function<void(wxCommandEvent&)> action, 
                         int id, const wxString& icon, const wxString& accel, wxItemKind kind)
{
    Item item;
    item.id = (id == wxID_ANY || id == -1) ? wxNewId() : id;
    item.label = label;
    item.help = help;
    item.action = action;
    item.icon = icon;
    item.accel = accel;
    item.isCheckable = (kind == wxITEM_CHECK);
    m_items.push_back(item);
    return &m_items.back();
}

void ThemedMenu::Clear()
{
    m_items.clear();
}

size_t ThemedMenu::GetMenuItemCount() const
{
    return m_items.size();
}

ThemedMenu::Item* ThemedMenu::FindItemByPosition(size_t pos)
{
    if (pos < m_items.size()) {
        return &m_items[pos];
    }
    return nullptr;
}

void ThemedMenu::Trigger(int id)
{
    for (auto& item : m_items) {
        if (item.id == id && !item.isSeparator) {
            if (item.action) {
                wxCommandEvent evt(wxEVT_MENU, id);
                item.action(evt);
            }
            return;
        }
        if (item.subMenu) {
            item.subMenu->Trigger(id);
        }
    }
}

} // namespace GUI
} // namespace Slic3r