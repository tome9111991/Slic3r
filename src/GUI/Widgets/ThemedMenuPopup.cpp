#include "ThemedMenuPopup.hpp"
#include "../Theme/ThemeManager.hpp"
#include <wx/dcbuffer.h>
#include <wx/app.h>

namespace Slic3r {
namespace GUI {

wxBEGIN_EVENT_TABLE(ThemedMenuPopup, wxPopupTransientWindow)
    EVT_ERASE_BACKGROUND(ThemedMenuPopup::OnEraseBackground)
    EVT_PAINT(ThemedMenuPopup::OnPaint)
    EVT_MOTION(ThemedMenuPopup::OnMotion)
    EVT_LEFT_UP(ThemedMenuPopup::OnLeftUp)
    EVT_LEAVE_WINDOW(ThemedMenuPopup::OnLeaveWindow)
    EVT_KEY_DOWN(ThemedMenuPopup::OnKeyDown)
wxEND_EVENT_TABLE()

void ThemedMenuPopup::OnEraseBackground(wxEraseEvent& event)
{
    // Do nothing to prevent flicker
}

ThemedMenuPopup::ThemedMenuPopup(wxWindow* parent, ThemedMenu* menu)
    : wxPopupTransientWindow(parent, wxBORDER_NONE), m_menu(menu)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_font = ThemeManager::GetFont(ThemeManager::FontSize::Small);
    
    // Parse our custom menu items
// Constructor loop change
    if (m_menu) {
        for (const auto& item : m_menu->GetItems()) {
            ItemInfo info;
            info.id = item.id;
            info.isSeparator = item.isSeparator;
            info.isEnabled = item.isEnabled;
            info.isChecked = item.isChecked;
            info.hasSubMenu = (item.subMenu != nullptr);
            info.subMenu = item.subMenu;
            info.label = item.label;
            info.icon = item.icon; // Copy icon
            info.accel = item.accel;
            
            m_items.push_back(info);
        }
    }

    CalculateLayout();
    PrecacheIcons();
}

void ThemedMenuPopup::PrecacheIcons()
{
    ThemeColors colors = ThemeManager::GetColors();
    wxColour textCol = colors.text;
    wxColour hoverCol = *wxWHITE; // Always white on highlight in our current theme

    for (auto& item : m_items) {
        if (!item.icon.IsEmpty() && !item.isSeparator) {
            // Normal state
            wxBitmapBundle bndlNormal = ThemeManager::GetSVG(item.icon, wxSize(16, 16), textCol);
            item.bundleNormal = bndlNormal.GetBitmap(wxSize(16, 16));

            // Hover state
            wxBitmapBundle bndlHover = ThemeManager::GetSVG(item.icon, wxSize(16, 16), hoverCol);
            item.bundleHover = bndlHover.GetBitmap(wxSize(16, 16));
        }
    }
}

ThemedMenuPopup::~ThemedMenuPopup()
{
    if (m_ownsMenu && m_menu) {
        delete m_menu;
    }
}

void ThemedMenuPopup::CalculateLayout()
{
    wxClientDC dc(this);
    dc.SetFont(m_font);

    int paddingX = 20; 
    int paddingY = 8;  
    int iconSpace = 25; 
    int accelSpace = 40; 
    int minWidth = 150;

    int maxWidthLabel = 0;
    int maxWidthAccel = 0;
    
    for (const auto& item : m_items) {
        if (item.isSeparator) continue;
        
        wxString cleanLabel = item.label;
        cleanLabel.Replace("&", ""); // Remove accelerator markers for size calc

        wxSize labelSize = dc.GetTextExtent(cleanLabel);
        wxSize accelSize = dc.GetTextExtent(item.accel);
        
        if (labelSize.x > maxWidthLabel) maxWidthLabel = labelSize.x;
        if (accelSize.x > maxWidthAccel) maxWidthAccel = accelSize.x;
    }

    int itemHeight = dc.GetTextExtent("Wg").y + paddingY;
    int sepHeight = 8;

    int totalWidth = iconSpace + maxWidthLabel + paddingX + maxWidthAccel + paddingX;
    if (totalWidth < minWidth) totalWidth = minWidth;
    
    int currentY = 5; 

    for (auto& item : m_items) {
        if (item.isSeparator) {
            item.rect = wxRect(0, currentY, totalWidth, sepHeight);
            currentY += sepHeight;
        } else {
            item.rect = wxRect(0, currentY, totalWidth, itemHeight);
            currentY += itemHeight;
        }
    }
    
    currentY += 5; 
    
    SetSize(totalWidth, currentY);
}

void ThemedMenuPopup::OnDismiss()
{
    if (m_onDismiss) {
        m_onDismiss();
    }
    wxPopupTransientWindow::OnDismiss();
    // Schedule destruction so we don't leak memory when created on heap
    wxTheApp->CallAfter([this]() {
        this->Destroy();
    });
}

bool ThemedMenuPopup::ProcessLeftDown(wxMouseEvent& event)
{
    return wxPopupTransientWindow::ProcessLeftDown(event);
}

void ThemedMenuPopup::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this);
    ThemeColors colors = ThemeManager::GetColors();

    wxColour bgCol = colors.header;
    if (colors.isDark) bgCol = bgCol.ChangeLightness(110);
    
    dc.SetBackground(wxBrush(bgCol));
    dc.Clear();
    
    dc.SetPen(wxPen(colors.isDark ? wxColour(80,80,80) : wxColour(200,200,200)));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(GetClientRect());

    dc.SetFont(m_font);

    wxRegion updateRegion = GetUpdateRegion();

    for (int i = 0; i < (int)m_items.size(); ++i) {
        const auto& item = m_items[i];
        
        // Skip items outside the update region for performance
        if (updateRegion.IsOk() && !updateRegion.IsEmpty() && !updateRegion.Contains(item.rect)) {
            continue;
        }

        if (item.isSeparator) {
            wxColour sepCol = colors.text;
            sepCol = sepCol.ChangeLightness(50); 
            dc.SetPen(wxPen(sepCol));
            int y = item.rect.GetTop() + item.rect.GetHeight()/2;
            dc.DrawLine(10, y, item.rect.GetWidth() - 10, y);
            continue;
        }

        wxColour textCol = colors.text;
        if (!item.isEnabled) textCol = colors.text.ChangeLightness(50);

        if (i == m_hoveredIndex && item.isEnabled) {
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(wxBrush(colors.accent)); 
            dc.DrawRectangle(item.rect);
            textCol = *wxWHITE; // White text on hover
        } else {
            // Already set default above
        }
        
        dc.SetTextForeground(textCol);

        int iconX = 5;
        int textX = 30;
        int accelX = item.rect.GetWidth() - 10;
        
        wxString cleanLabel = item.label;
        cleanLabel.Replace("&", ""); // Remove accelerator markers for display

        int centerY = item.rect.GetTop() + (item.rect.GetHeight() - dc.GetTextExtent(cleanLabel).y) / 2;

        // Draw Icon OR Checkbox
        if (item.isChecked) {
            wxPoint pts[3] = {
                wxPoint(iconX + 3, item.rect.GetTop() + item.rect.GetHeight()/2),
                wxPoint(iconX + 7, item.rect.GetTop() + item.rect.GetHeight()/2 + 4),
                wxPoint(iconX + 13, item.rect.GetTop() + item.rect.GetHeight()/2 - 4)
            };
            dc.SetPen(wxPen(textCol, 2)); // Use text color (white if hovered, normal otherwise)
            dc.DrawLines(3, pts);
        } else if (item.bundleNormal.IsOk()) {
            // Use cached bitmaps
            wxBitmap iconBmp = (i == m_hoveredIndex && item.isEnabled) ? item.bundleHover : item.bundleNormal;
            
            if (iconBmp.IsOk()) {
                // Center the icon vertically in the row
                int iconY = item.rect.GetTop() + (item.rect.GetHeight() - 16) / 2;
                dc.DrawBitmap(iconBmp, iconX, iconY, true);
            }
        }

        if (item.hasSubMenu) {
             wxPoint pts[3] = {
                wxPoint(accelX - 5, item.rect.GetTop() + item.rect.GetHeight()/2 - 4),
                wxPoint(accelX - 5, item.rect.GetTop() + item.rect.GetHeight()/2 + 4),
                wxPoint(accelX,     item.rect.GetTop() + item.rect.GetHeight()/2)
            };
            dc.SetBrush(wxBrush(textCol)); // Use text color
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawPolygon(3, pts);
        }

        dc.DrawText(cleanLabel, textX, centerY);

        if (!item.accel.IsEmpty()) {
            wxSize s = dc.GetTextExtent(item.accel);
            dc.DrawText(item.accel, accelX - s.x - 10, centerY);
        }
    }
}


void ThemedMenuPopup::OnMotion(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    int prevIndex = m_hoveredIndex;
    int newIndex = -1;

    for (int i = 0; i < (int)m_items.size(); ++i) {
        if (m_items[i].rect.Contains(pos) && !m_items[i].isSeparator) {
            newIndex = i;
            break;
        }
    }

    if (prevIndex != newIndex) {
        m_hoveredIndex = newIndex;
        // Optimization: Reset old item and set new item without full refresh if possible
        if (prevIndex != -1)
            RefreshRect(m_items[prevIndex].rect);
        
        if (newIndex != -1)
            RefreshRect(m_items[newIndex].rect);
            
        // If we moved into/out of valid items, just refreshing those reqcts is enough
        // because the background logic is handled in Paint for each rect
    }
}

void ThemedMenuPopup::OnLeaveWindow(wxMouseEvent& event)
{
    m_hoveredIndex = -1;
    Refresh();
}

void ThemedMenuPopup::OnLeftUp(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    for (const auto& item : m_items) {
        if (item.rect.Contains(pos)) {
            if (item.isEnabled && !item.isSeparator) {
                ExecuteItem(item);
            }
            return; 
        }
    }
}

void ThemedMenuPopup::OnKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_ESCAPE) {
        Dismiss();
    }
    event.Skip();
}

void ThemedMenuPopup::ExecuteItem(const ItemInfo& item)
{
    if (item.hasSubMenu) {
        // Recursively show submenu
        ThemedMenuPopup* sub = new ThemedMenuPopup(this, item.subMenu.get());
        
        // Pass callbacks down to submenus so actions inside them also trigger the reset
        sub->SetOnDismissCallback(m_onDismiss);

        wxPoint screenPos = ClientToScreen(wxPoint(item.rect.GetRight(), item.rect.GetTop()));
        sub->Position(screenPos, wxSize(0,0));
        sub->Popup();
        return;
    }

    int cmdId = item.id;
    Dismiss(); 
    
    // Direct Execution! No event loop detour.
    // Defer to next event loop iteration to allow the menu to close and repaint first
    if (m_menu) {
        // Capture necessary data. m_menu is owned by ThemedMenuBar which should outlive this transient moment
        ThemedMenu* menu = m_menu; 
        wxTheApp->CallAfter([menu, cmdId]() {
            menu->Trigger(cmdId);
        });
    }
}

} // namespace GUI
} // namespace Slic3r