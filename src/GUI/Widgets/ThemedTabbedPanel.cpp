#include "ThemedTabbedPanel.hpp"
#include "../Theme/ThemeManager.hpp"
#include <wx/dcbuffer.h>

namespace Slic3r { namespace GUI {

ThemedTabbedPanel::ThemedTabbedPanel(wxWindow* parent, wxWindowID id, 
                                     const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, id, pos, size, wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    
    // Create the central content area (simple book)
    m_book = new wxSimplebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    
    // We will draw the tabs ourselves on the main panel
    Bind(wxEVT_PAINT, &ThemedTabbedPanel::OnPaint, this);
    Bind(wxEVT_SIZE, &ThemedTabbedPanel::OnResize, this);
    Bind(wxEVT_LEFT_DOWN, &ThemedTabbedPanel::OnMouseLeftDown, this);
    Bind(wxEVT_MOTION, &ThemedTabbedPanel::OnMouseMotion, this);
    Bind(wxEVT_LEAVE_WINDOW, &ThemedTabbedPanel::OnMouseLeave, this);
    Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) { });
}

wxScrolledWindow* ThemedTabbedPanel::AddTab(const wxString& label, const wxBitmapBundle& icon) {
    // Create a new scrolled window as the "page"
    wxScrolledWindow* page = new wxScrolledWindow(m_book, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    
    // Standard setup for a setting list
    page->SetBackgroundColour(ThemeManager::GetColors().bg);
    page->SetScrollRate(0, 10); // Standard vertical scrolling
    
    // Each page gets a vertical sizer by default so user can just Add() controls
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    page->SetSizer(sizer);
    
    // Add to book
    m_book->AddPage(page, label);
    
    // Store metadata
    TabEntry newTab;
    newTab.label = label;
    newTab.icon = icon;
    newTab.page = page;
    m_tabs.push_back(newTab);
    
    // If first tab, select it
    if (m_selection == -1) {
        SetSelection(0);
    }
    
    Refresh(); // Redraw tabs
    return page;
}

void ThemedTabbedPanel::SetSelection(int n) {
    if (n >= 0 && n < (int)m_tabs.size()) {
        m_selection = n;
        m_book->ChangeSelection(n); // Switch visible page
        Refresh(); // Redraw tabs to show active state
        
        // Fire event
        wxCommandEvent e(wxEVT_NOTEBOOK_PAGE_CHANGED, GetId());
        e.SetInt(m_selection);
        e.SetEventObject(this);
        GetEventHandler()->ProcessEvent(e);
    }
}

int ThemedTabbedPanel::GetHeaderHeight() const {
    double scale = GetContentScaleFactor();
    // Default header height: 38px scaled (compact with icons)
    return (int)(38 * scale);
}

void ThemedTabbedPanel::OnResize(wxSizeEvent& evt) {
    wxSize sz = GetClientSize();
    int headerH = GetHeaderHeight();
    
    // Resize the book to fill the area below the header
    if (m_book) {
        m_book->SetSize(0, headerH, sz.x, sz.y - headerH);
        m_book->Layout(); // Ensure the active page inside resizes
    }
    
    evt.Skip();
}

void ThemedTabbedPanel::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    auto theme = ThemeManager::GetColors();
    dc.SetBackground(wxBrush(theme.bg));
    dc.Clear();
    
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;
    
    double scale = GetContentScaleFactor();
    int headerH = GetHeaderHeight();
    wxSize sz = GetClientSize();

    // 1. Draw Header Background
    // Consistent with rest of app headers
    wxColour headerBg = theme.isDark ? theme.bg.ChangeLightness(105) : theme.bg.ChangeLightness(95);
    gc->SetBrush(wxBrush(headerBg));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRectangle(0, 0, sz.x, headerH);
    
    // Draw bottom border of header
    gc->SetPen(wxPen(theme.border, 1));
    gc->StrokeLine(0, headerH, sz.x, headerH);
    
    // 2. Draw Tabs
    double currentX = 10 * scale;
    int tabHeight = headerH; 
    
    gc->SetFont(ThemeManager::GetFont(ThemeManager::FontSize::Small), theme.text);

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        double textW, textH;
        gc->GetTextExtent(m_tabs[i].label, &textW, &textH);
        
        bool hasIcon = m_tabs[i].icon.IsOk();
        double iconSize = 16.0 * scale; // Smaller Icon
        double iconPadding = 6.0 * scale; 
        
        double contentW = textW;
        if (hasIcon) {
            contentW += iconSize + iconPadding;
        }

        double tabW = contentW + (20 * scale); // 10px padding each side
        
        // Store hit rect for mouse interaction
        m_tabs[i].hitRect = wxRect((int)currentX, 0, (int)tabW, tabHeight);
        
        bool isActive = ((int)i == m_selection);
        bool isHover = ((int)i == m_hoverIndex);
        
        if (isActive) {
             // Active tab style: Accent Underline + Accent Text
             gc->SetPen(wxPen(theme.accent, 2 * scale)); 
             gc->StrokeLine(currentX, headerH - (1 * scale), currentX + tabW, headerH - (1 * scale));
             
             // Initial active font setup
             gc->SetFont(ThemeManager::GetFont(ThemeManager::FontSize::Small, ThemeManager::FontWeight::Bold), theme.accent);
        } else {
             if (isHover) {
                 // Hover: Slight background highlight
                 wxColour hoverBg = theme.surface.ChangeLightness(110);
                 gc->SetBrush(wxBrush(hoverBg));
                 gc->SetPen(*wxTRANSPARENT_PEN);
                 gc->DrawRoundedRectangle(currentX, 2 * scale, tabW, tabHeight - (2*scale), 4 * scale);
             }
             gc->SetFont(ThemeManager::GetFont(ThemeManager::FontSize::Small), theme.textMuted);
        }
        
        // alignment center
        double contentX = currentX + (tabW - contentW) / 2.0;
        double contentY = (tabHeight - textH) / 2.0; 
        
        if (hasIcon) {
            // Draw Icon
            wxBitmap bmp = m_tabs[i].icon.GetBitmap(wxSize(iconSize, iconSize));
            if (bmp.IsOk()) {
                 double iconY = (tabHeight - iconSize) / 2.0;
                 gc->DrawBitmap(bmp, contentX, iconY, iconSize, iconSize);
            }
            contentX += iconSize + iconPadding;
        }

        // Draw Text
        gc->DrawText(m_tabs[i].label, contentX, contentY);
        
        currentX += tabW + (2 * scale); // Gap between tabs
    }
}

void ThemedTabbedPanel::OnMouseMotion(wxMouseEvent& evt) {
    wxPoint pos = evt.GetPosition();
    int oldHover = m_hoverIndex;
    m_hoverIndex = -1;
    
    // Optimization: only check if Y is within header height
    if (pos.y <= GetHeaderHeight()) {
        for (size_t i = 0; i < m_tabs.size(); ++i) {
            if (m_tabs[i].hitRect.Contains(pos)) {
                m_hoverIndex = (int)i;
                break;
            }
        }
    }
    
    if (m_hoverIndex != oldHover) {
        RefreshRect(wxRect(0, 0, GetSize().x, GetHeaderHeight()));
    }
    evt.Skip();
}

void ThemedTabbedPanel::OnMouseLeave(wxMouseEvent& evt) {
    if (m_hoverIndex != -1) {
        m_hoverIndex = -1;
        RefreshRect(wxRect(0, 0, GetSize().x, GetHeaderHeight()));
    }
    evt.Skip();
}

void ThemedTabbedPanel::OnMouseLeftDown(wxMouseEvent& evt) {
    wxPoint pos = evt.GetPosition();
    
    // Check if we clicked a header
    if (pos.y <= GetHeaderHeight()) {
        for (size_t i = 0; i < m_tabs.size(); ++i) {
            if (m_tabs[i].hitRect.Contains(pos)) {
                if ((int)i != m_selection) {
                    SetSelection((int)i);
                }
                break;
            }
        }
    }
    evt.Skip();
}

wxSize ThemedTabbedPanel::DoGetBestSize() const {
    double scale = GetContentScaleFactor();
    return wxSize(400 * scale, 300 * scale); 
}

void ThemedTabbedPanel::Clear() {
    m_book->DeleteAllPages();
    m_tabs.clear();
    m_selection = -1;
    m_hoverIndex = -1;
    Refresh();
}

}} // namespace Slic3r::GUI
