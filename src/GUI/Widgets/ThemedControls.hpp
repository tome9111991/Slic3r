#ifndef THEMEDCONTROLS_HPP
#define THEMEDCONTROLS_HPP

#include <wx/wx.h>
#include <wx/graphics.h>
#include "../Theme/ThemeManager.hpp"

namespace Slic3r { namespace GUI {

// A. Universal Button
class ThemedButton : public wxControl {
public:
    ThemedButton(wxWindow* parent, wxWindowID id, const wxString& label, 
                 const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(100, 35));

private:
    void OnPaint(wxPaintEvent& evt);
    
    wxString m_label;
    bool m_hover = false;
    bool m_pressed = false;
};

// B. SVG Checkbox
class ThemedCheckBox : public wxControl {
public:
    ThemedCheckBox(wxWindow* parent, wxWindowID id, const wxString& label,
                   const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
    
    bool IsChecked() const { return m_checked; }
    void SetValue(bool val);

private:
    void OnPaint(wxPaintEvent& evt);
    
    bool m_checked = false;
    wxString m_label;
};

// C. Dropdown / Select
class ThemedSelect : public wxControl {
public:
    ThemedSelect(wxWindow* parent, wxWindowID id, const wxArrayString& options,
                 const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(150, 30));

    wxString GetValue() const { return m_current; }
    void SetValue(const wxString& val);

private:
    void OpenPopup(wxMouseEvent& evt);
    void OnPaint(wxPaintEvent& evt);

    wxArrayString m_options;
    wxString m_current;
};

}} // namespace Slic3r::GUI

#endif // THEMEDCONTROLS_HPP
