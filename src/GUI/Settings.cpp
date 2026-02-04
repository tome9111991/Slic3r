#include "Settings.hpp"
#include "GUI.hpp"
#include "misc_ui.hpp"
#include <wx/fileconf.h> 
#include <wx/display.h>

namespace Slic3r { namespace GUI {

std::unique_ptr<Settings> ui_settings {nullptr}; 

Settings::Settings() {
    // Initialize fonts using ThemeManager
    this->_small_font = ThemeManager::GetFont(ThemeManager::FontSize::Small);
    this->_small_bold_font = ThemeManager::GetFont(ThemeManager::FontSize::Small, ThemeManager::FontWeight::Bold);
    this->_medium_font = ThemeManager::GetFont(ThemeManager::FontSize::Medium);

    this->_scroll_step = this->_small_font.GetPointSize();
}
void Settings::save_settings() {
    if (!SLIC3RAPP) return;
    wxString path = SLIC3RAPP->datadir + "/slic3r.ini";
    wxFileConfig config("Slic3r", "Slic3r", path, "", wxCONFIG_USE_LOCAL_FILE);

    // Save General Preferences
    config.Write("version_check", this->version_check);
    config.Write("remember_output_path", this->remember_output_path);
    config.Write("autocenter", this->autocenter);
    config.Write("autoalignz", this->autoalignz);
    config.Write("invert_zoom", this->invert_zoom);
    config.Write("background_processing", this->background_processing);
    config.Write("threads", (long)this->threads);
    config.Write("tabbed_preset_editors", this->preset_editor_tabs);
    config.Write("show_host", this->show_host);
    config.Write("reload_hide_dialog", this->hide_reload_dialog);
    config.Write("reload_preserve_trafo", this->reload_preserve_trafo);
    config.Write("dark_mode", this->dark_mode);
    config.Write("reload_behavior", (long)this->reload);
    config.Write("rotation_controls", wxString(this->rotation_controls));

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

    // Load General Preferences
    config.Read("version_check", &this->version_check, true);
    config.Read("remember_output_path", &this->remember_output_path, true);
    config.Read("autocenter", &this->autocenter, true);
    config.Read("autoalignz", &this->autoalignz, true);
    config.Read("invert_zoom", &this->invert_zoom, false);
    config.Read("background_processing", &this->background_processing, false);
    
    long t_threads;
    if (config.Read("threads", &t_threads)) this->threads = (unsigned int)t_threads;
    
    config.Read("tabbed_preset_editors", &this->preset_editor_tabs, true);
    config.Read("show_host", &this->show_host, true);
    config.Read("reload_hide_dialog", &this->hide_reload_dialog, false);
    config.Read("reload_preserve_trafo", &this->reload_preserve_trafo, false);
    config.Read("dark_mode", &this->dark_mode, true);
    this->apply_theme();
    
    long t_reload;
    if (config.Read("reload_behavior", &t_reload)) this->reload = (ReloadBehavior)t_reload;

    wxString t_rot;
    if (config.Read("rotation_controls", &t_rot)) this->rotation_controls = t_rot.ToStdString();

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

// Pure Internal Theme Switching + Simple Native Flag
void Settings::apply_theme() {
    // ThemeManager::SetDarkMode will trigger UpdateUI() which handles 
    // coloring all windows, including MainFrame and Dialogs.
    ThemeManager::SetDarkMode(this->dark_mode);

#ifdef _WIN32
    if (SLIC3RAPP) SLIC3RAPP->MSWForceDarkMode(this->dark_mode);
#endif
}

void Settings::apply_theme_to_window(wxWindow* win) {
     if (!win) return;
     // Forward to centralized logic
     ThemeManager::ApplyThemeRecursive(win);
}

}} // namespace Slic3r::GUI
