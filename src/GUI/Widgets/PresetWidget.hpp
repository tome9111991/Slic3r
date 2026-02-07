#ifndef PRESET_WIDGET_HPP
#define PRESET_WIDGET_HPP

#include <wx/wx.h>
#include <wx/graphics.h>
#include <functional>
#include "ThemedControls.hpp"
#include "../Theme/ThemeManager.hpp"

namespace Slic3r { namespace GUI {

class PresetSection : public wxPanel {
public:
    PresetSection(wxWindow* parent, const wxString& title, const wxString& icon_name);

    void SetValue(const wxString& value);
    wxString GetValue() const;
    void UpdateTheme();

    ThemedSelect* GetSelector() const { return m_selector; }

    wxSize DoGetBestSize() const override;

    std::function<void()> on_selector_click;
    std::function<void()> on_settings_click;
    std::function<void()> on_edit_click;

private:
    void OnPaint(wxPaintEvent& event);
    void OnMouseEvent(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);

    wxString m_title;
    wxString m_icon_name;

    ThemedSelect* m_selector = nullptr;

    bool m_hover_selector = false;
    bool m_hover_settings = false;
    bool m_hover_edit = false;

    wxRect m_settings_rect;
    wxRect m_edit_rect;
};

}} // namespace Slic3r::GUI

#endif // PRESET_WIDGET_HPP
