#include "ThemedControls.hpp"
#include <wx/dcbuffer.h>
#include <wx/menu.h>

namespace Slic3r { namespace GUI {

// --- ThemedButton ---

ThemedButton::ThemedButton(wxWindow* parent, wxWindowID id, const wxString& label, const wxPoint& pos, const wxSize& size)
    : wxControl(parent, id, pos, size, wxBORDER_NONE), m_label(label) 
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    
    Bind(wxEVT_PAINT, &ThemedButton::OnPaint, this);
    Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& e) { m_hover = true; Refresh(); e.Skip(); });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& e) { m_hover = false; m_pressed = false; Refresh(); e.Skip(); });
    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) { m_pressed = true; Refresh(); e.Skip(); });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e) { 
        if(m_pressed) {
            wxCommandEvent evt(wxEVT_BUTTON, GetId());
            evt.SetEventObject(this);
            GetEventHandler()->ProcessEvent(evt);
        }
        m_pressed = false; Refresh(); 
        e.Skip();
    });
}

void ThemedButton::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    auto theme = ThemeManager::GetColors();
    
    // Draw background to match parent if needed, or rely on Clear() if parent has same bg
    dc.SetBackground(wxBrush(theme.bg));
    dc.Clear();

    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (gc) {
        // Background color based on state
        wxColour currentBg = theme.surface;
        if (m_pressed) {
            currentBg = theme.accent.ChangeLightness(90);
        } else if (m_hover) {
            currentBg = theme.accent.ChangeLightness(110);
        }

        gc->SetBrush(wxBrush(currentBg));
        gc->SetPen(wxPen(theme.border, 1));
        gc->DrawRoundedRectangle(0, 0, GetSize().x, GetSize().y, 5);

        // Text
        gc->SetFont(GetFont(), theme.text);
        double tw, th;
        gc->GetTextExtent(m_label, &tw, &th);
        gc->DrawText(m_label, (GetSize().x - tw) / 2, (GetSize().y - th) / 2);
        delete gc;
    }
}

wxSize ThemedButton::DoGetBestSize() const {
    wxClientDC dc(const_cast<ThemedButton*>(this));
    dc.SetFont(GetFont());
    wxSize s = dc.GetTextExtent(m_label);
    return wxSize(s.GetWidth() + 30, s.GetHeight() + 16);
}

// --- ThemedCheckBox ---

ThemedCheckBox::ThemedCheckBox(wxWindow* parent, wxWindowID id, const wxString& label, const wxPoint& pos, const wxSize& size)
    : wxControl(parent, id, pos, size, wxBORDER_NONE), m_label(label)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &ThemedCheckBox::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) { 
        m_checked = !m_checked; 
        
        wxCommandEvent evt(wxEVT_CHECKBOX, GetId());
        evt.SetInt(m_checked ? 1 : 0);
        evt.SetEventObject(this);
        GetEventHandler()->ProcessEvent(evt);
        
        Refresh(); 
        e.Skip();
    });
}

void ThemedCheckBox::SetValue(bool val) {
    if (m_checked != val) {
        m_checked = val;
        Refresh();
    }
}

void ThemedCheckBox::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(ThemeManager::GetColors().bg));
    dc.Clear();
    
    auto theme = ThemeManager::GetColors();
    
    // Choose icon and color
    wxString iconName = m_checked ? "checkbox_checked" : "checkbox_unchecked";
    wxColour iconColor = m_checked ? theme.accent : theme.textMuted;
    
    // Load and Draw SVG (14x14 fits better with small text)
    wxBitmapBundle bundle = ThemeManager::GetSVG(iconName, wxSize(14, 14), iconColor);
    
    // Center vertically
    int yPos = (GetSize().y - 14) / 2;
    // Ensure we don't draw off-canvas if squeezed
    if (yPos < 0) yPos = 0;
    
    if (bundle.IsOk()) {
        dc.DrawBitmap(bundle.GetBitmapFor(this), 0, yPos, true);
    }

    // Text (if any)
    if (!m_label.IsEmpty()) {
        dc.SetTextForeground(theme.text);
        dc.SetFont(GetFont());
        wxSize extent = dc.GetTextExtent(m_label);
        dc.DrawText(m_label, 18, (GetSize().y - extent.y) / 2);
    }
}

wxSize ThemedCheckBox::DoGetBestSize() const {
    // Return enough height to fit the 14px icon comfortably
    if (m_label.IsEmpty()) return wxSize(18, 18);
    
    wxClientDC dc(const_cast<ThemedCheckBox*>(this));
    dc.SetFont(GetFont());
    wxSize text = dc.GetTextExtent(m_label);
    // Height should be max of icon(14) or text, plus some padding
    int h = std::max(18, text.GetHeight() + 2);
    return wxSize(18 + text.GetWidth() + 8, h);
}

// --- ThemedSelect ---

ThemedSelect::ThemedSelect(wxWindow* parent, wxWindowID id, const wxArrayString& options, const wxPoint& pos, const wxSize& size)
    : wxControl(parent, id, pos, size, wxBORDER_NONE), m_options(options)
{
    if (!options.IsEmpty()) m_current = options[0];
    
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &ThemedSelect::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &ThemedSelect::OpenPopup, this);
}

void ThemedSelect::SetValue(const wxString& val) {
    m_current = val;
    Refresh();
}

void ThemedSelect::OpenPopup(wxMouseEvent&) {
    wxMenu menu;
    for (size_t i = 0; i < m_options.size(); ++i) {
        // Use a base ID + index. 
        // Note: In a real complex app, you might want better ID handling to avoid collisions
        int itemId = 20000 + (int)i; 
        menu.Append(itemId, m_options[i]);
        
        // We can't easily bind to the specific ID here with a lambda in a loop in standard wxWidgets 
        // without careful capture, but Bind works on the menu object or window.
        // Easiest is to bind the range to the window processing the menu.
        // But here we are in a control.
        
        // Actually, PopupMenu blocks. So we can handle events? No, on wxWidgets it returns immediately or blocks depending on platform.
        // Better: Bind to the menu itself if supported, or use a specific event handler.
    }
    
    // Handle menu selection
    menu.Bind(wxEVT_MENU, [this](wxCommandEvent& e) {
        int idx = e.GetId() - 20000;
        if (idx >= 0 && idx < (int)m_options.size()) {
            m_current = m_options[idx];
            Refresh();
            
            wxCommandEvent evt(wxEVT_COMBOBOX, GetId());
            evt.SetString(m_current);
            evt.SetInt(idx);
            evt.SetEventObject(this);
            GetEventHandler()->ProcessEvent(evt);
        }
    });

    PopupMenu(&menu);
}

void ThemedSelect::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    auto theme = ThemeManager::GetColors();
    dc.SetBackground(wxBrush(theme.bg));
    dc.Clear();

    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (gc) {
        gc->SetPen(wxPen(theme.border, 1));
        gc->SetBrush(wxBrush(theme.surface));
        gc->DrawRoundedRectangle(0, 0, GetSize().x, GetSize().y, 3);
        
        // Text
        gc->SetFont(GetFont(), theme.text);
        double tw, th;
        gc->GetTextExtent(m_current, &tw, &th);
        
        // Left align with padding
        gc->DrawText(m_current, 8, (GetSize().y - th) / 2);
        
        delete gc;
    }
    
    // Arrow Icon
    wxBitmapBundle arrow = ThemeManager::GetSVG("arrow_down", wxSize(10, 10), theme.text);
    if (arrow.IsOk()) {
        dc.DrawBitmap(arrow.GetBitmapFor(this), GetSize().x - 18, (GetSize().y - 10) / 2, true);
    } else {
        // Fallback arrow
        dc.SetPen(wxPen(theme.text, 2));
        dc.DrawLine(GetSize().x - 15, 10, GetSize().x - 10, 18);
        dc.DrawLine(GetSize().x - 10, 18, GetSize().x - 5, 10);
    }
}

wxSize ThemedSelect::DoGetBestSize() const {
    return wxSize(150, 30);
}

}} // namespace Slic3r::GUI
