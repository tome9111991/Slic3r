#include "Settings.hpp"
#include "GUI.hpp"
#include "misc_ui.hpp"
#include <wx/fileconf.h> 
#include <wx/display.h>

namespace Slic3r { namespace GUI {

std::unique_ptr<Settings> ui_settings {nullptr}; 

Settings::Settings() {
    // Initialize fonts
    this->_small_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    if (the_os == OS::Mac) _small_font.SetPointSize(11);
    this->_small_bold_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    if (the_os == OS::Mac) _small_bold_font.SetPointSize(11);
    this->_small_bold_font.MakeBold();
    this->_medium_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    this->_medium_font.SetPointSize(12);

    this->_scroll_step = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize();
}
void Settings::save_settings() {
    if (!SLIC3RAPP) return;
    wxString path = SLIC3RAPP->datadir + "/slic3r.ini";
    wxFileConfig config("Slic3r", "Slic3r", path, "", wxCONFIG_USE_LOCAL_FILE);

    // Save window positions
    for (auto const& [name, tuple] : window_pos) {
        config.Write(name + "_pos_x", std::get<0>(tuple).x);
        config.Write(name + "_pos_y", std::get<0>(tuple).y);
        config.Write(name + "_width", std::get<1>(tuple).x);
        config.Write(name + "_height", std::get<1>(tuple).y);
        config.Write(name + "_maximized", std::get<2>(tuple));
    }
    config.Flush();
}

void Settings::load_settings() {
    if (!SLIC3RAPP) return;
    wxString path = SLIC3RAPP->datadir + "/slic3r.ini";
    if (!wxFileExists(path)) return;

    wxFileConfig config("Slic3r", "Slic3r", path, "", wxCONFIG_USE_LOCAL_FILE);

    auto load_win = [&](wxString name) {
        long x, y, w, h;
        bool max;
        if (config.Read(name + "_pos_x", &x) &&
            config.Read(name + "_pos_y", &y) &&
            config.Read(name + "_width", &w) &&
            config.Read(name + "_height", &h)) {

            config.Read(name + "_maximized", &max, false);
            window_pos[name] = std::make_tuple(wxPoint(x, y), wxSize(w, h), max);
        }
    };

    load_win("main_frame");
}

void Settings::save_window_pos(wxWindow* ref, wxString name) {
    wxTopLevelWindow* parent = dynamic_cast<wxTopLevelWindow*>(ref);
    if (!parent) return;

    window_pos[name] = std::make_tuple(
        parent->GetScreenPosition(),
        parent->GetSize(),
        parent->IsMaximized()
    );
    this->save_settings();
}

void Settings::restore_window_pos(wxWindow* ref, wxString name) {
    if (window_pos.find(name) == window_pos.end()) return;

    auto t = window_pos[name];
    wxPoint pos = std::get<0>(t);
    wxSize size = std::get<1>(t);
    bool max = std::get<2>(t);

    wxTopLevelWindow* parent = dynamic_cast<wxTopLevelWindow*>(ref);
    if (!parent) return;

    parent->SetSize(size);
    
    // Ensure the window is within screen bounds
    wxDisplay display(wxDisplay::GetFromWindow(parent));
    if (display.IsOk()) {
        wxRect screen = display.GetClientArea();
        if (!screen.Contains(pos)) {
            // If strictly outside, maybe center it? 
            // Or just check if top-left is visible?
            // Simple check:
            if (pos.x < screen.GetLeft() || pos.x > screen.GetRight() || 
                pos.y < screen.GetTop() || pos.y > screen.GetBottom()) {
                parent->Center();
            } else {
                parent->Move(pos);
            }
        } else {
            parent->Move(pos);
        }
    } else {
        parent->Move(pos);
    }

    if (max) parent->Maximize(true);
}
}} // namespace Slic3r::GUI
