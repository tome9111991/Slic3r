#include "ThemedMenuBar.hpp"
#include "ThemedMenuPopup.hpp"
#include "../Theme/ThemeManager.hpp"
#include <wx/dcbuffer.h>
#include <wx/app.h>

namespace Slic3r {
namespace GUI {

wxBEGIN_EVENT_TABLE(ThemedMenuBar, wxPanel)
    EVT_PAINT(ThemedMenuBar::OnPaint)
    EVT_LEFT_UP(ThemedMenuBar::OnLeftUp)
    EVT_MOTION(ThemedMenuBar::OnMotion)
    EVT_LEAVE_WINDOW(ThemedMenuBar::OnLeaveWindow)
wxEND_EVENT_TABLE()

ThemedMenuBar::ThemedMenuBar(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, 30)); // Ensure it has height
}

ThemedMenuBar::~ThemedMenuBar()
{
    for (auto& entry : m_menus) {
        delete entry.menu;
    }
    m_menus.clear();
}

void ThemedMenuBar::Clear()
{
    if (m_currentPopup) {
        m_currentPopup->Dismiss();
        m_currentPopup = nullptr;
    }

    for (auto& entry : m_menus) {
        delete entry.menu;
    }
    m_menus.clear();
    m_hoveredIndex = -1;
    m_openMenuIndex = -1;
    
    Refresh();
}

bool ThemedMenuBar::Append(ThemedMenu* menu, const wxString& title)
{
    if (!menu) return false;
    
    wxString label = title;
    label.Replace("&", "");

    MenuEntry entry;
    entry.menu = menu;
    entry.title = label; 
    m_menus.push_back(entry);
    
    Refresh();
    return true;
}

void ThemedMenuBar::UpdateTheme()
{
    Refresh();
}

void ThemedMenuBar::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this);
    ThemeColors colors = ThemeManager::GetColors();

    dc.SetBackground(wxBrush(colors.header)); 
    dc.Clear();

    dc.SetFont(ThemeManager::GetFont(ThemeManager::FontSize::Small));

    int x = 10; 
    int itemPadding = 15; 
    
    int height = GetSize().GetHeight();

    for (size_t i = 0; i < m_menus.size(); ++i) {
        wxString label = m_menus[i].title;
        wxSize textSize = dc.GetTextExtent(label);

        int textY = (height - textSize.y) / 2;
        wxRect itemRect(x - 5, 0, textSize.x + 10, height);
        m_menus[i].rect = itemRect;

        if (static_cast<int>(i) == m_hoveredIndex || static_cast<int>(i) == m_openMenuIndex) {
            dc.SetPen(wxPen(colors.surface));
            dc.SetBrush(wxBrush(colors.surface)); 
            dc.DrawRectangle(itemRect);
        }

        dc.SetTextForeground(colors.text);
        dc.DrawText(label, x, textY);

        x += textSize.x + itemPadding;
    }
}

void ThemedMenuBar::OnLeftUp(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    int menuIndex = -1;

    for (size_t i = 0; i < m_menus.size(); ++i) {
        if (m_menus[i].rect.Contains(pos)) {
            menuIndex = static_cast<int>(i);
            break; 
        }
    }

    if (menuIndex != -1) {
        // If clicking the ALREADY OPEN menu, close it
        if (menuIndex == m_openMenuIndex) {
            if (m_currentPopup) {
                m_currentPopup->Dismiss();
                // Callback will handle reset
            }
        } else {
            OpenMenu(menuIndex);
        }
    }
}

void ThemedMenuBar::OnMotion(wxMouseEvent& event)
{
    if (m_isResetting) return;

    wxPoint pos = event.GetPosition();
    int prevIndex = m_hoveredIndex;
    m_hoveredIndex = -1;

    for (size_t i = 0; i < m_menus.size(); ++i) {
        if (m_menus[i].rect.Contains(pos)) {
            m_hoveredIndex = static_cast<int>(i);
            break;
        }
    }

    if (prevIndex != m_hoveredIndex) {
        Refresh();
    }

    // Auto-switch if a menu is already open and we hover over a different one
    if (m_openMenuIndex != -1 && m_hoveredIndex != -1 && m_hoveredIndex != m_openMenuIndex) {
        OpenMenu(m_hoveredIndex);
    }
}

void ThemedMenuBar::OpenMenu(int index)
{
    if (index < 0 || index >= static_cast<int>(m_menus.size())) return;

    // Close existing popup if any
    if (m_currentPopup) {
         // Prevent callback from triggering full reset since we are effectively replacing it
         m_currentPopup->SetOnDismissCallback(nullptr);
         m_currentPopup->Dismiss(); 
         m_currentPopup = nullptr;
    }

    MenuEntry& entry = m_menus[index];
    
    m_openMenuIndex = index;
    Refresh();

    ThemedMenuPopup* popup = new ThemedMenuPopup(this, entry.menu);
    
    popup->SetOnDismissCallback([this]() {
        m_isResetting = true;
        m_openMenuIndex = -1;
        m_hoveredIndex = -1; 
        m_currentPopup = nullptr;
        
        Refresh();
        Update(); 

        // Also force parent update if possible, to ensure the repaint is visible before any modal dialogs
        if (GetParent()) {
            GetParent()->Update();
        }

        // Defer clearing the resetting flag to allow event queue to settle
        wxTheApp->CallAfter([this]() {
            m_isResetting = false;
        });
    });

    wxPoint screenPos = ClientToScreen(wxPoint(entry.rect.GetLeft(), entry.rect.GetBottom()));
    popup->Position(screenPos, wxSize(0,0)); 
    popup->Popup();
    m_currentPopup = popup;
}

void ThemedMenuBar::OnLeaveWindow(wxMouseEvent& event)
{
    if (m_hoveredIndex != -1) {
        m_hoveredIndex = -1;
        Refresh();
    }
}

wxSize ThemedMenuBar::DoGetBestSize() const
{
    return wxSize(-1, 30);
}

} // namespace GUI
} // namespace Slic3r