#ifndef THEMEDCONTROLS_HPP
#define THEMEDCONTROLS_HPP

#include <wx/wx.h>
#include <wx/graphics.h>
#include "../Theme/ThemeManager.hpp"

namespace Slic3r { namespace GUI {

// A. Universal Button (Replacement for wxButton)
class ThemedButton : public wxControl {
public:
    ThemedButton(wxWindow* parent, wxWindowID id, const wxString& label, 
                 const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(100, 35));

    // Optional icon support
    void SetBitmap(const wxBitmapBundle& bmp);

    wxSize DoGetBestSize() const override;

private:
    void OnPaint(wxPaintEvent& evt);
    
    wxString m_label;
    wxBitmapBundle m_icon;
    bool m_hover = false;
    bool m_pressed = false;
};

// B. SVG Checkbox (Replacement for wxCheckBox)
class ThemedCheckBox : public wxControl {
public:
    ThemedCheckBox(wxWindow* parent, wxWindowID id, const wxString& label,
                   const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
    
    bool IsChecked() const { return m_checked; }
    void SetValue(bool val);

    wxSize DoGetBestSize() const override;

private:
    void OnPaint(wxPaintEvent& evt);
    
    bool m_checked = false;
    wxString m_label;
};

// C. Dropdown / Select (Replacement for wxChoice or wxComboBox)
class ThemedSelect : public wxControl {
public:
    ThemedSelect(wxWindow* parent, wxWindowID id, const wxArrayString& options = wxArrayString(),
                 const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(150, 30));

    wxString GetValue() const { return m_current; }
    void SetValue(const wxString& val);

    int GetSelection() const;
    void SetSelection(int n);
    
    // Compatibility helpers
    void Clear();
    void Append(const wxString& item, const wxBitmapBundle& icon = wxBitmapBundle());
    void SetItemIcon(int n, const wxBitmapBundle& icon);
    size_t GetCount() const { return m_options.size(); }
    int FindString(const wxString& s) const;
    void SetString(int n, const wxString& s);
    wxString GetString(int n) const;

    void SetFlat(bool flat) { m_isFlat = flat; Refresh(); }

    wxSize DoGetBestSize() const override;

private:
    void OpenPopup(wxMouseEvent& evt);
    void OnPaint(wxPaintEvent& evt);

    wxArrayString m_options;
    std::vector<wxBitmapBundle> m_icons;
    wxString m_current;
    bool m_popupOpen = false;
    bool m_hover = false;
    bool m_isFlat = false;
};

// D. Text Input Wrapper (Replacement for single-line wxTextCtrl)
class ThemedTextInput : public wxControl {
public:
    ThemedTextInput(wxWindow* parent, wxWindowID id, const wxString& value = "",
                    const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(200, 30));

    wxString GetValue() const;
    void SetValue(const wxString& val);

    wxTextCtrl* GetTextCtrl() const { return m_textCtrl; }

    wxSize DoGetBestSize() const override;

private:
    void OnPaint(wxPaintEvent& evt);
    void OnSize(wxSizeEvent& evt);
    
    wxTextCtrl* m_textCtrl;
};

// E. Multi-line Text Area (Replacement for multi-line wxTextCtrl)
class ThemedTextArea : public wxControl {
public:
    ThemedTextArea(wxWindow* parent, wxWindowID id, const wxString& value = "",
                   const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(400, 150));

    wxString GetValue() const;
    void SetValue(const wxString& val);

    wxTextCtrl* GetTextCtrl() const { return m_textCtrl; }

    wxSize DoGetBestSize() const override;

private:
    void OnPaint(wxPaintEvent& evt);
    void OnSize(wxSizeEvent& evt);
    
    wxTextCtrl* m_textCtrl;
};

// F. Number Input (Replacement for wxSpinCtrl or wxSpinCtrlDouble)
class ThemedNumberInput : public wxControl {
public:
    ThemedNumberInput(wxWindow* parent, wxWindowID id, double value = 0.0,
                      const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(100, 30));

    double GetValue() const;
    void SetValue(double val);
    
    void SetRange(double min, double max);
    void SetIncrement(double inc);

    wxSize DoGetBestSize() const override;

private:
    void OnPaint(wxPaintEvent& evt);
    void OnSize(wxSizeEvent& evt);
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnMouseLeave(wxMouseEvent& evt);
    void OnTextEnter(wxCommandEvent& evt);
    
    // Updates value text and clamps range
    void UpdateText();

    wxTextCtrl* m_textCtrl;
    double m_value;
    double m_min = 0.0;
    double m_max = 1000.0;
    double m_inc = 1.0;
    
    // Button state
    enum class ButtonState { None, UpHover, DownHover, UpPressed, DownPressed };
    ButtonState m_btnState = ButtonState::None;
    
    wxRect m_upRect;
    wxRect m_downRect;
    
    void FireChangeEvent();
};

// G. Themed Section Panel (Replacement for wxStaticBox)
class ThemedSection : public wxPanel {
public:
    ThemedSection(wxWindow* parent, const wxString& title, const wxString& icon = "");

    // Add content to the returned sizer
    wxBoxSizer* GetContentSizer() const { return m_contentSizer; }
    
    void SetTitle(const wxString& title) { m_title = title; Refresh(); }

    wxSize DoGetBestSize() const override;

private:
    void OnPaint(wxPaintEvent& evt);
    void OnSize(wxSizeEvent& evt);
    
    wxString m_title;
    wxString m_iconName;
    wxBoxSizer* m_contentSizer;
};

// H. Generic Themed Panel (with optional border)
class ThemedPanel : public wxPanel {
public:
    ThemedPanel(wxWindow* parent, wxWindowID id = wxID_ANY, 
                const wxPoint& pos = wxDefaultPosition, 
                const wxSize& size = wxDefaultSize);

    void SetBorder(bool visible, const wxColour& color = wxNullColour);
    
private:
    void OnPaint(wxPaintEvent& evt);
    
    bool m_hasBorder = false;
    wxColour m_borderColor;
};

}} // namespace Slic3r::GUI

#endif // THEMEDCONTROLS_HPP
