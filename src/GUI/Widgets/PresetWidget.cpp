#include "PresetWidget.hpp"
#include "../Theme/ThemeManager.hpp"
#include "misc_ui.hpp"
#include <wx/dcbuffer.h>

namespace Slic3r { namespace GUI {

PresetSection::PresetSection(wxWindow* parent, const wxString& title, const wxString& icon_name)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTAB_TRAVERSAL | wxCLIP_CHILDREN),
      m_title(title), m_icon_name(icon_name)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    
    m_selector = new ThemedSelect(this, wxID_ANY);
    m_selector->SetFlat(true);
    
    Bind(wxEVT_PAINT, &PresetSection::OnPaint, this);
    Bind(wxEVT_MOTION, &PresetSection::OnMouseEvent, this);
    Bind(wxEVT_LEFT_DOWN, &PresetSection::OnMouseEvent, this);
    Bind(wxEVT_LEFT_UP, &PresetSection::OnMouseEvent, this);
    Bind(wxEVT_SIZE, &PresetSection::OnSize, this);
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { 
        if (m_hover_settings || m_hover_edit) {
            m_hover_settings = m_hover_edit = false; 
            RefreshRect(m_settings_rect);
            Update(); 
        }
    });
    Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) { });
}

void PresetSection::SetValue(const wxString& value) {
    if (m_selector) {
        m_selector->SetValue(value);
    }
}

wxString PresetSection::GetValue() const {
    return m_selector ? m_selector->GetValue() : wxString("");
}


void PresetSection::UpdateTheme() {
    if (m_selector) m_selector->Refresh();
    Refresh();
}

void PresetSection::OnSize(wxSizeEvent& event) {
    double scale = GetContentScaleFactor();
    wxSize sz = GetClientSize();
    
    int headerH = 25 * scale;
    int selectorY = 2 * scale + headerH + 2 * scale;
    int selectorH = 32 * scale;
    
    if (m_selector) {
        m_selector->SetSize(5 * scale, selectorY, sz.x - 10 * scale, selectorH);
    }
    
    // Update hit rects based on new size
    m_settings_rect = wxRect(sz.x - 30 * scale, 2 * scale, 25 * scale, headerH);
    
    event.Skip();
}

void PresetSection::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    auto theme = ThemeManager::GetColors();
    dc.SetBackground(wxBrush(theme.bg));

    // Smart Background Clearing:
    // Only clear the panel background if we are painting outside the header area.
    // If we only refresh an icon inside the header, clearing with panel-bg causes flickering.
    double scale = GetContentScaleFactor();
    int headerY = 2 * scale;
    int headerH = 25 * scale;

    wxRect updateRect = GetUpdateRegion().GetBox();
    bool insideHeader = (updateRect.y >= headerY && (updateRect.y + updateRect.height) <= (headerY + headerH));

    if (!insideHeader || updateRect.IsEmpty()) {
        dc.Clear();
    }

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;

    wxSize sz = GetSize();
    
    // 1. Draw Header Bar
    // headerY and headerH are already defined above
    
    // Draw background for the full header bar
    wxColour headerBg = theme.isDark ? wxColour(50, 50, 50) : wxColour(235, 235, 235);
    gc->SetBrush(wxBrush(headerBg));
    gc->SetPen(*wxTRANSPARENT_PEN);
    // Draw across the full width (sz.x)
    gc->DrawRoundedRectangle(0, headerY, sz.x, headerH, 4 * scale);

    // Header Icon
    if (!m_icon_name.IsEmpty()) {
        wxBitmapBundle bundle = ThemeManager::GetSVG(m_icon_name, wxSize(16, 16), theme.text);
        if (bundle.IsOk()) {
            wxBitmap bmp = bundle.GetBitmapFor(this);
            gc->DrawBitmap(bmp, 8 * scale, headerY + (headerH - bmp.GetHeight()) / 2.0, bmp.GetWidth(), bmp.GetHeight());
        }
    }
    
    // Header Title
    gc->SetFont(ThemeManager::GetFont(ThemeManager::FontSize::Small, ThemeManager::FontWeight::Bold), theme.text);
    double tw, th;
    gc->GetTextExtent(m_title, &tw, &th);
    gc->DrawText(m_title, 28 * scale, headerY + (headerH - th) / 2.0);
    
    // Settings Icon (on the right, inside the bar)
    m_settings_rect = wxRect(sz.x - 30 * scale, headerY, 25 * scale, headerH);
    {
        wxColour iconCol = m_hover_settings ? theme.accent : theme.textMuted;
        wxBitmapBundle bundle = ThemeManager::GetSVG("cog.svg", wxSize(16, 16), iconCol);
        if (bundle.IsOk()) {
            wxBitmap bmp = bundle.GetBitmapFor(this);
            gc->DrawBitmap(bmp, m_settings_rect.x + (m_settings_rect.width - bmp.GetWidth()) / 2.0, 
                           m_settings_rect.y + (m_settings_rect.height - bmp.GetHeight()) / 2.0, bmp.GetWidth(), bmp.GetHeight());
        }
    }

    // selector is handled by the child control m_selector
}

void PresetSection::OnMouseEvent(wxMouseEvent& event) {
    wxPoint pos = event.GetPosition();
    bool hover_settings = m_settings_rect.Contains(pos);
    bool hover_edit = m_edit_rect.Contains(pos);
    
    if (hover_settings != m_hover_settings || hover_edit != m_hover_edit) {
        m_hover_settings = hover_settings;
        m_hover_edit = hover_edit;
        RefreshRect(m_settings_rect);
        Update(); 
    }
    
    if (event.LeftDown()) {
        if (m_hover_settings && on_settings_click) on_settings_click();
        else if (m_hover_edit && on_edit_click) on_edit_click();
    }
    
    event.Skip();
}

wxSize PresetSection::DoGetBestSize() const {
    double scale = GetContentScaleFactor();
    // Header (25) + Padding (2) + Selector (32) + Bottom Padding (5)
    return wxSize(300 * scale, 64 * scale);
}

}} // namespace Slic3r::GUI
