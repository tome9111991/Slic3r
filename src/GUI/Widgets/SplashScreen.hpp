#ifndef SPLASHSCREEN_HPP
#define SPLASHSCREEN_HPP

#include <wx/wx.h>
#include <wx/graphics.h>
#include "../Theme/ThemeManager.hpp"

namespace Slic3r { namespace GUI {

class SplashScreen : public wxFrame {
public:
    SplashScreen(const wxString& title, const wxString& version);

    void SetStatus(const wxString& status);

private:
    void OnPaint(wxPaintEvent& evt);
    void OnEraseBackground(wxEraseEvent& evt) { /* Do nothing to prevent flicker */ }

    wxString m_version;
    wxString m_status;
    wxBitmapBundle m_logo;
};

}} // namespace Slic3r::GUI

#endif // SPLASHSCREEN_HPP
