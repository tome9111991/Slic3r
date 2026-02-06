#include "ThemedControls.hpp"
#include "../Theme/ThemeManager.hpp"
#include <wx/dcbuffer.h>
#include <wx/menu.h>
#include <wx/popupwin.h>

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



void ThemedButton::SetBitmap(const wxBitmapBundle& bmp) {
    m_icon = bmp;
    Refresh();
}

void ThemedButton::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    auto theme = ThemeManager::GetColors();
    
    // Draw background to match parent if needed
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
        
        double scale = GetContentScaleFactor();
        gc->DrawRoundedRectangle(0, 0, GetSize().x, GetSize().y, 5 * scale);

        // Calculate content layout
        double contentW = 0;
        double gap = 6 * scale;
        
        // Measure text
        double tw = 0, th = 0;
        if (!m_label.IsEmpty()) {
            gc->SetFont(GetFont(), theme.text);
            gc->GetTextExtent(m_label, &tw, &th);
            contentW += tw;
        }

        // Measure icon
        wxBitmap iconBmp;
        if (m_icon.IsOk()) {
            // Use height of button minus padding for icon size target? 
            // Or just use natural size (usually 16x16)
            iconBmp = m_icon.GetBitmapFor(this);
            contentW += iconBmp.GetWidth();
            if (!m_label.IsEmpty()) contentW += gap;
        }

        // Draw Content Centered
        double x = (GetSize().x - contentW) / 2.0;
        double cy = GetSize().y / 2.0;

        if (iconBmp.IsOk()) {
             double iy = cy - (iconBmp.GetHeight() / 2.0);
             // GraphicsContext DrawBitmap allows drawing wxBitmap
             gc->DrawBitmap(iconBmp, x, iy, iconBmp.GetWidth(), iconBmp.GetHeight());
             x += iconBmp.GetWidth() + gap;
        }

        if (!m_label.IsEmpty()) {
             double ty = cy - (th / 2.0);
             gc->DrawText(m_label, x, ty);
        }

        delete gc;
    }
}

wxSize ThemedButton::DoGetBestSize() const {
    wxClientDC dc(const_cast<ThemedButton*>(this));
    dc.SetFont(GetFont());
    wxSize s = dc.GetTextExtent(m_label);
    double scale = GetContentScaleFactor();
    
    int w = s.GetWidth();
    int h = s.GetHeight();
    
    if (m_icon.IsOk()) {
        wxBitmap bmp = m_icon.GetBitmap(wxSize(16,16)); // Probe size
        w += bmp.GetWidth();
        if (!m_label.IsEmpty()) w += (6 * scale);
        h = std::max(h, bmp.GetHeight());
    }
    
    return wxSize(w + (30 * scale), h + (16 * scale));
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
    
    double scale = GetContentScaleFactor();

    // Choose icon and color
    wxString iconName = m_checked ? "checkbox_checked" : "checkbox_unchecked";
    wxColour iconColor = m_checked ? theme.accent : theme.textMuted;
    
    // Load and Draw SVG (14x14 fits better with small text)
    wxBitmapBundle bundle = ThemeManager::GetSVG(iconName, wxSize(14, 14), iconColor);
    
    if (bundle.IsOk()) {
        wxBitmap bmp = bundle.GetBitmapFor(this);
        // Center the icon horizontally in the 18*scale reserved box and vertically in the control height
        int xPos = std::max(0, (int)((18.0 * scale - bmp.GetWidth()) / 2.0));
        int yPos = std::max(0, (GetSize().y - bmp.GetHeight()) / 2);
        
        dc.DrawBitmap(bmp, xPos, yPos, true);
    }

    // Text (if any)
    if (!m_label.IsEmpty()) {
        dc.SetTextForeground(theme.text);
        dc.SetFont(GetFont());
        wxSize extent = dc.GetTextExtent(m_label);
        dc.DrawText(m_label, 18 * scale, (GetSize().y - extent.y) / 2);
    }
}

wxSize ThemedCheckBox::DoGetBestSize() const {
    double scale = GetContentScaleFactor();
    // Return enough height to fit the icon comfortably
    if (m_label.IsEmpty()) return wxSize(18 * scale, 18 * scale);
    
    wxClientDC dc(const_cast<ThemedCheckBox*>(this));
    dc.SetFont(GetFont());
    wxSize text = dc.GetTextExtent(m_label);
    // Height should be max of icon(14 scaled) or text, plus some padding
    int h = std::max((int)(18 * scale), text.GetHeight() + (int)(2 * scale));
    return wxSize((18 * scale) + text.GetWidth() + (8 * scale), h);
}

// --- ThemedSelect ---

ThemedSelect::ThemedSelect(wxWindow* parent, wxWindowID id, const wxArrayString& options, const wxPoint& pos, const wxSize& size)
    : wxControl(parent, id, pos, size, wxBORDER_NONE), m_options(options)
{
    // Don't auto-select first item if it might be empty or valid to not select? 
    // wxChoice usually starts with -1 (no selection) unless SetSelection is called.
    // But for dropdown, we usually want checking.
    if (!options.IsEmpty()) m_current = options[0];
    
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &ThemedSelect::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &ThemedSelect::OpenPopup, this);
}

void ThemedSelect::SetValue(const wxString& val) {
    m_current = val;
    Refresh();
}

int ThemedSelect::GetSelection() const {
    if (m_options.IsEmpty()) return wxNOT_FOUND;
    for (size_t i = 0; i < m_options.size(); ++i) {
        if (m_options[i] == m_current) return (int)i;
    }
    return wxNOT_FOUND;
}

void ThemedSelect::SetSelection(int n) {
    if (n == wxNOT_FOUND) {
        m_current = "";
    } else if (n >= 0 && n < (int)m_options.size()) {
        m_current = m_options[n];
    }
    Refresh();
}

void ThemedSelect::Clear() {
    m_options.Clear();
    m_icons.clear();
    m_current = "";
    Refresh();
}

void ThemedSelect::Append(const wxString& item, const wxBitmapBundle& icon) {
    m_options.Add(item);
    // Pad m_icons if it was lagging (shouldn't happen if consistently using Append)
    while (m_icons.size() < m_options.size() - 1) m_icons.push_back(wxBitmapBundle());
    m_icons.push_back(icon);
    
    if (m_options.GetCount() == 1 && m_current.IsEmpty()) {
        m_current = item; // Auto-select first if previously empty?
    }
    Refresh();
}

void ThemedSelect::SetItemIcon(int n, const wxBitmapBundle& icon) {
    if (n >= 0 && n < (int)m_icons.size()) {
        m_icons[n] = icon;
        Refresh();
    }
}

wxString ThemedSelect::GetString(int n) const {
    if (n >= 0 && n < (int)m_options.size()) return m_options[n];
    return "";
}

int ThemedSelect::FindString(const wxString& s) const {
    for (size_t i = 0; i < m_options.size(); ++i) {
        if (m_options[i] == s) return (int)i;
    }
    return wxNOT_FOUND;
}

void ThemedSelect::SetString(int n, const wxString& s) {
    if (n >= 0 && n < (int)m_options.size()) {
        m_options[n] = s;
        if (m_current == m_options[n]) { // Update current if it matched? 
            // Actually if we rename the selected item, m_current (which is string based) might surely mismatch if we don't update it?
            // But m_current tracks the VALUE. If we change the option in the list, does the selected value change?
            // Usually SetString changes the label.
            // If the renamed item WAS selected, we should probably update m_current to the new string so it stays valid.
        }
        // Actually, logic: check if index n is the currently selected index.
        if (GetSelection() == n) {
             m_current = s;
        }
        Refresh();
    }
}

// --- ThemedSelectPopup ---
// Internal helper class for ThemedSelect's dropdown
class ThemedSelectPopup : public wxPopupTransientWindow {
public:
    ThemedSelectPopup(wxWindow* parent, const wxArrayString& options, const std::vector<wxBitmapBundle>& icons, const wxString& current, std::function<void(int)> onDismiss)
        : wxPopupTransientWindow(parent, wxBORDER_NONE), m_options(options), m_icons(icons), m_current(current), m_onDismiss(onDismiss)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        
        // Find current index
        for (size_t i = 0; i < m_options.size(); ++i) {
            if (m_options[i] == m_current) {
                m_hoverIndex = (int)i;
                break;
            }
        }

        // Calculate size
        double scale = GetContentScaleFactor();
        int rowH = 26 * scale;
        int totalH = (int)m_options.size() * rowH + (int)(2 * scale); // 1px padding top/bottom
        
        // Match parent width
        int w = parent->GetSize().x;
        SetSize(w, totalH);

        Bind(wxEVT_PAINT, &ThemedSelectPopup::OnPaint, this);
        Bind(wxEVT_MOTION, &ThemedSelectPopup::OnMotion, this);
        Bind(wxEVT_LEFT_UP, &ThemedSelectPopup::OnMouseClick, this);
        Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&){ /* do nothing */ });
    }

    int GetSelection() const { return m_selection; }

    void OnDismiss() override {
        if (m_onDismiss) m_onDismiss(m_selection);
        wxPopupTransientWindow::OnDismiss();
        wxTheApp->CallAfter([this]() {
            this->Destroy();
        });
    }

private:
    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        // dc.Clear() is not needed as we fill the whole area with gc
        
        auto theme = ThemeManager::GetColors();
        double scale = GetContentScaleFactor();
        int rowH = 26 * scale;
        int paddingY = 1 * scale;
        
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (gc) {
            // 1. Draw background
            gc->SetBrush(wxBrush(theme.bg));
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawRectangle(0, 0, GetSize().x, GetSize().y);
            
            // 2. Draw items
            gc->SetFont(ThemeManager::GetFont(ThemeManager::FontSize::Small), theme.text);
            
            for (size_t i = 0; i < m_options.size(); ++i) {
                int rowY = paddingY + (int)i * rowH;
                
                // Highlight frame
                if ((int)i == m_hoverIndex) {
                    gc->SetBrush(wxBrush(theme.accent.ChangeLightness(180))); // Subtle highlight background
                    gc->SetPen(wxPen(theme.accent, 1));
                    gc->DrawRectangle(1 * scale, rowY, GetSize().x - 2 * scale, rowH);
                }

                // Icon
                int tx = 8 * scale;
                if (i < m_icons.size() && m_icons[i].IsOk()) {
                    wxBitmap bmp = m_icons[i].GetBitmapFor(this);
                    double iy = rowY + (rowH - bmp.GetHeight()) / 2.0;
                    gc->DrawBitmap(bmp, tx, iy, bmp.GetWidth(), bmp.GetHeight());
                    tx += bmp.GetWidth() + 6 * scale;
                }

                // Label
                double tw, th;
                gc->GetTextExtent(m_options[i], &tw, &th);
                double ty = rowY + (rowH - th) / 2.0;
                gc->DrawText(m_options[i], tx, ty);
            }

            // 3. Border (draw last to be on top)
            gc->SetPen(wxPen(theme.border, 1));
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->DrawRectangle(0, 0, GetSize().x, GetSize().y);
        }
    }

    void OnMotion(wxMouseEvent& evt) {
        double scale = GetContentScaleFactor();
        int rowH = 26 * scale;
        int paddingY = 1 * scale;
        
        int y = evt.GetPosition().y;
        int newHover = -1;
        if (y >= paddingY) {
            newHover = (y - paddingY) / rowH;
        }
        
        if (newHover >= (int)m_options.size() || newHover < 0) newHover = -1;

        if (newHover != m_hoverIndex) {
            int oldHover = m_hoverIndex;
            m_hoverIndex = newHover;

            // Only refresh the affected rows to prevent flicker
            if (oldHover != -1) {
                RefreshRect(wxRect(0, paddingY + oldHover * rowH, GetSize().x, rowH));
            }
            if (m_hoverIndex != -1) {
                RefreshRect(wxRect(0, paddingY + m_hoverIndex * rowH, GetSize().x, rowH));
            }
        }
    }

    void OnMouseClick(wxMouseEvent& evt) {
        if (m_hoverIndex != -1) {
            m_selection = m_hoverIndex;
            Dismiss();
            
            // Trigger parent event
            wxCommandEvent event(wxEVT_COMBOBOX, GetParent()->GetId());
            event.SetInt(m_selection);
            event.SetString(m_options[m_selection]);
            event.SetEventObject(GetParent());
            GetParent()->GetEventHandler()->ProcessEvent(event);
        }
    }

    wxArrayString m_options;
    std::vector<wxBitmapBundle> m_icons;
    wxString m_current;
    int m_hoverIndex = -1;
    int m_selection = -1;
    std::function<void(int)> m_onDismiss;
};



void ThemedSelect::OpenPopup(wxMouseEvent&) {
    if (m_options.IsEmpty()) return;

    m_popupOpen = true;
    Refresh();

    ThemedSelectPopup* popup = new ThemedSelectPopup(this, m_options, m_icons, m_current, [this](int sel) {
        if (sel != wxNOT_FOUND) {
            m_current = m_options[sel];
        }
        m_popupOpen = false;
        Refresh();
    });
    
    // Position directly below the control
    wxPoint screenPos = ClientToScreen(wxPoint(0, GetSize().y));
    popup->Position(screenPos, wxSize(0,0));
    
    popup->Popup();
}

void ThemedSelect::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    auto theme = ThemeManager::GetColors();
    
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (gc) {
        double scale = GetContentScaleFactor();
        
        // Background and Border
        wxColour borderColor = m_popupOpen ? theme.accent : theme.border;
        gc->SetPen(wxPen(borderColor, 1));
        gc->SetBrush(wxBrush(theme.surface));
        gc->DrawRoundedRectangle(0, 0, GetSize().x, GetSize().y, 3 * scale);
        
        // Text and Icon
        gc->SetFont(GetFont(), theme.text);
        double tw, th;
        gc->GetTextExtent(m_current, &tw, &th);
        
        double x = 8 * scale;
        
        // Find current icon
        wxBitmap iconBmp;
        for(size_t i=0; i<m_options.size(); i++) {
            if (m_options[i] == m_current && i < m_icons.size()) {
                 iconBmp = m_icons[i].GetBitmapFor(this);
                 break;
            }
        }
        
        if (iconBmp.IsOk()) {
             double iy = (GetSize().y - iconBmp.GetHeight()) / 2.0;
             gc->DrawBitmap(iconBmp, (int)x, (int)iy, iconBmp.GetWidth(), iconBmp.GetHeight());
             x += iconBmp.GetWidth() + (6 * scale);
        }
        
        // Selection Text
        gc->DrawText(m_current, (int)x, (int)((GetSize().y - th) / 2.0));

        // Arrow Icon
        wxBitmapBundle arrow = ThemeManager::GetSVG("arrow_down", wxSize(10, 10), theme.text);
        if (arrow.IsOk()) {
            wxBitmap bmp = arrow.GetBitmapFor(this);
            int margin = 18 * scale;
            int yPos = (GetSize().y - bmp.GetHeight()) / 2;
            gc->DrawBitmap(bmp, GetSize().x - margin, yPos, bmp.GetWidth(), bmp.GetHeight());
        } else {
            // Fallback arrow
            gc->SetPen(wxPen(theme.text, 2 * scale));
            int margin = 15 * scale;
            int sz = 5 * scale;
            double cy = GetSize().y / 2.0;
            double topY = cy - 2 * scale;
            
            gc->StrokeLine(GetSize().x - margin, topY, GetSize().x - (margin - sz), topY + sz);
            gc->StrokeLine(GetSize().x - (margin - sz), topY + sz, GetSize().x - (margin - 2*sz), topY);
        }
    }
}

wxSize ThemedSelect::DoGetBestSize() const {
    double scale = GetContentScaleFactor();
    return wxSize(150 * scale, 30 * scale);
}


// --- ThemedTextInput ---

ThemedTextInput::ThemedTextInput(wxWindow* parent, wxWindowID id, const wxString& value, const wxPoint& pos, const wxSize& size)
    : wxControl(parent, id, pos, size, wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    
    // Create inner native text control
    // wxTE_PROCESS_ENTER allows handling Enter key if needed
    m_textCtrl = new wxTextCtrl(this, wxID_ANY, value, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_PROCESS_ENTER);
    
    auto theme = ThemeManager::GetColors();
    m_textCtrl->SetBackgroundColour(theme.surface);
    m_textCtrl->SetForegroundColour(theme.text);
    
    Bind(wxEVT_PAINT, &ThemedTextInput::OnPaint, this);
    Bind(wxEVT_SIZE, &ThemedTextInput::OnSize, this);
    
    // Repaint on focus change to update border color
    m_textCtrl->Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& e) { Refresh(); e.Skip(); });
    m_textCtrl->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { Refresh(); e.Skip(); });
    
    // Clicking the wrapper should focus the text input
    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) { m_textCtrl->SetFocus(); e.Skip(); });
}

wxString ThemedTextInput::GetValue() const {
    return m_textCtrl->GetValue();
}

void ThemedTextInput::SetValue(const wxString& val) {
    m_textCtrl->SetValue(val);
}

void ThemedTextInput::OnSize(wxSizeEvent& evt) {
    wxSize size = GetClientSize();
    double scale = GetContentScaleFactor();
    
    // Horizontal padding (left/right)
    int padX = 8 * scale;
    
    // Calculate best height for text control to center it vertically
    wxSize bestSz = m_textCtrl->GetBestSize();
    int targetH = bestSz.y;
    
    int yPos = (size.y - targetH) / 2;
    if (yPos < 0) yPos = 0;
    
    // Resize inner control
    // Width is full width minus padding on both sides
    int targetW = size.x - (2 * padX);
    if (targetW < 0) targetW = 0;
    
    m_textCtrl->SetSize(padX, yPos, targetW, targetH);
    
    evt.Skip();
}

void ThemedTextInput::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    auto theme = ThemeManager::GetColors();
    dc.SetBackground(wxBrush(theme.bg));
    dc.Clear();
    
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (gc) {
        double scale = GetContentScaleFactor();
        
        // Border color: Accent if focused, otherwise standard border
        wxColour borderColor = theme.border;
        if (m_textCtrl->HasFocus()) {
            borderColor = theme.accent;
        }
        
        gc->SetBrush(wxBrush(theme.surface));
        gc->SetPen(wxPen(borderColor, 1));
        
        // Draw background and border
        // We draw full size
        gc->DrawRoundedRectangle(0, 0, GetSize().x, GetSize().y, 3 * scale);
        
        delete gc;
    }
}

wxSize ThemedTextInput::DoGetBestSize() const {
    double scale = GetContentScaleFactor();
    wxSize innerBest = m_textCtrl->GetBestSize();
    
    // Add padding to inner best size
    // 16px horizontal padding total (8+8), 10px vertical (5+5)
    return wxSize(150 * scale, innerBest.y + (10 * scale));
}


// --- ThemedNumberInput ---

ThemedNumberInput::ThemedNumberInput(wxWindow* parent, wxWindowID id, double value, const wxPoint& pos, const wxSize& size)
    : wxControl(parent, id, pos, size, wxBORDER_NONE), m_value(value)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    // Create inner native text control
    m_textCtrl = new wxTextCtrl(this, wxID_ANY, wxString::Format("%.2f", value), 
                                wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_PROCESS_ENTER);

    auto theme = ThemeManager::GetColors();
    m_textCtrl->SetBackgroundColour(theme.surface);
    m_textCtrl->SetForegroundColour(theme.text);

    Bind(wxEVT_PAINT, &ThemedNumberInput::OnPaint, this);
    Bind(wxEVT_SIZE, &ThemedNumberInput::OnSize, this);
    Bind(wxEVT_LEFT_DOWN, &ThemedNumberInput::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &ThemedNumberInput::OnLeftUp, this);
    Bind(wxEVT_LEAVE_WINDOW, &ThemedNumberInput::OnMouseLeave, this);
    
    // Repaint on focus changes
    m_textCtrl->Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& e) { Refresh(); e.Skip(); });
    m_textCtrl->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { 
        // Commit value on focus loss
        double val;
        if(m_textCtrl->GetValue().ToDouble(&val)) {
            SetValue(val);
        } else {
            UpdateText(); // Revert to valid value
        }
        Refresh(); 
        e.Skip(); 
    });
    
    // Handle Enter key
    m_textCtrl->Bind(wxEVT_TEXT, [this](wxCommandEvent& e) { e.Skip(); }); // Propagate text changes?
    m_textCtrl->Bind(wxEVT_TEXT_ENTER, &ThemedNumberInput::OnTextEnter, this);
    
    UpdateText();
}

double ThemedNumberInput::GetValue() const {
    return m_value;
}

void ThemedNumberInput::SetValue(double val) {
    if (val < m_min) val = m_min;
    if (val > m_max) val = m_max;
    m_value = val;
    UpdateText();
}

void ThemedNumberInput::SetRange(double min, double max) {
    m_min = min;
    m_max = max;
    SetValue(m_value); // Re-clamp
}

void ThemedNumberInput::SetIncrement(double inc) {
    m_inc = inc;
}

void ThemedNumberInput::UpdateText() {
    m_textCtrl->ChangeValue(wxString::Format("%.2f", m_value));
}

void ThemedNumberInput::OnTextEnter(wxCommandEvent& evt) {
    double val;
    if (m_textCtrl->GetValue().ToDouble(&val)) {
        SetValue(val);
        
        // Fire event
        wxCommandEvent e(wxEVT_NULL, GetId()); // Needs custom event type usually
        e.SetEventObject(this);
        // For now just process it locally, assuming user polls GetValue or we add event later
    } else {
        UpdateText();
    }
    evt.Skip();
}

void ThemedNumberInput::OnSize(wxSizeEvent& evt) {
    wxSize size = GetClientSize();
    double scale = GetContentScaleFactor();

    int btnWidth = 24 * scale;
    int padX = 8 * scale;
    
    wxSize bestSz = m_textCtrl->GetBestSize();
    int targetH = bestSz.y;
    int yPos = (size.y - targetH) / 2;
    if (yPos < 0) yPos = 0;
    
    int textW = size.x - btnWidth - padX - (1 * scale); // Reserve space for buttons + divider
    if (textW < 0) textW = 0;
    
    m_textCtrl->SetSize(padX, yPos, textW, targetH);
    
    // Define Button Rects
    int btnH = size.y / 2;
    m_upRect = wxRect(size.x - btnWidth, 0, btnWidth, btnH);
    m_downRect = wxRect(size.x - btnWidth, btnH, btnWidth, size.y - btnH);
    
    evt.Skip();
}

void ThemedNumberInput::OnLeftDown(wxMouseEvent& evt) {
    wxPoint pt = evt.GetPosition();
    if (m_upRect.Contains(pt)) {
        m_btnState = ButtonState::UpPressed;
        SetValue(m_value + m_inc);
        FireChangeEvent();
        Refresh();
    } else if (m_downRect.Contains(pt)) {
        m_btnState = ButtonState::DownPressed;
        SetValue(m_value - m_inc);
        FireChangeEvent();
        Refresh();
    } else {
        // Focus text if clicked elsewhere (outside inner control but on panel)
        m_textCtrl->SetFocus();
    }
}

void ThemedNumberInput::OnLeftUp(wxMouseEvent& evt) {
    m_btnState = ButtonState::None;
    Refresh();
}

void ThemedNumberInput::OnMouseLeave(wxMouseEvent& evt) {
    if (m_btnState != ButtonState::None) {
        m_btnState = ButtonState::None;
        Refresh();
    }
}

void ThemedNumberInput::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    auto theme = ThemeManager::GetColors();
    dc.SetBackground(wxBrush(theme.bg));
    dc.Clear();
    
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (gc) {
        double scale = GetContentScaleFactor();
        
        // Focus Color
        wxColour borderColor = theme.border;
        if (m_textCtrl->HasFocus()) {
            borderColor = theme.accent;
        }
        
        // Background
        gc->SetBrush(wxBrush(theme.surface));
        gc->SetPen(wxPen(borderColor, 1));
        gc->DrawRoundedRectangle(0, 0, GetSize().x, GetSize().y, 3 * scale);
        
        // Divider line for buttons
        gc->SetPen(wxPen(theme.border, 1));
        gc->StrokeLine(m_upRect.GetLeft(), 0, m_upRect.GetLeft(), GetSize().y);
        
        // Horizontal divider between up/down
        gc->StrokeLine(m_upRect.GetLeft(), m_upRect.GetHeight(), m_upRect.GetRight(), m_upRect.GetHeight());
        
        // Draw Button Backgrounds if pressed
        if (m_btnState == ButtonState::UpPressed) {
            gc->SetBrush(wxBrush(theme.accent.ChangeLightness(140)));
            gc->DrawRectangle(m_upRect.GetX(), m_upRect.GetY(), m_upRect.GetWidth(), m_upRect.GetHeight());
        } else if (m_btnState == ButtonState::DownPressed) {
            gc->SetBrush(wxBrush(theme.accent.ChangeLightness(140)));
            gc->DrawRectangle(m_downRect.GetX(), m_downRect.GetY(), m_downRect.GetWidth(), m_downRect.GetHeight());
        }

        // Draw Arrows
        wxSize iconSz(8, 8);
        wxBitmapBundle upIcon = ThemeManager::GetSVG("arrow_up", iconSz, theme.text);
        wxBitmapBundle downIcon = ThemeManager::GetSVG("arrow_down", iconSz, theme.text);
        
        if (upIcon.IsOk()) {
             wxBitmap bmp = upIcon.GetBitmapFor(this);
             gc->DrawBitmap(bmp, m_upRect.GetX() + (m_upRect.GetWidth() - bmp.GetWidth()) / 2.0, 
                           m_upRect.GetY() + (m_upRect.GetHeight() - bmp.GetHeight()) / 2.0, bmp.GetWidth(), bmp.GetHeight());
        }
        
        if (downIcon.IsOk()) {
             wxBitmap bmp = downIcon.GetBitmapFor(this);
             gc->DrawBitmap(bmp, m_downRect.GetX() + (m_downRect.GetWidth() - bmp.GetWidth()) / 2.0, 
                           m_downRect.GetY() + (m_downRect.GetHeight() - bmp.GetHeight()) / 2.0, bmp.GetWidth(), bmp.GetHeight());
        }
    }
}

wxSize ThemedNumberInput::DoGetBestSize() const {
    double scale = GetContentScaleFactor();
    wxSize inner = m_textCtrl->GetBestSize();
    // Text width + Button width + padding
    return wxSize(120 * scale, inner.y + (10 * scale));
}

void ThemedNumberInput::FireChangeEvent() {
    wxCommandEvent event(wxEVT_TEXT, GetId());
    event.SetEventObject(this);
    event.SetString(wxString::Format("%.2f", m_value));
    GetEventHandler()->ProcessEvent(event);
}

}} // namespace Slic3r::GUI
