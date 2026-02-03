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
    ThemeManager::SetDarkMode(this->dark_mode);

#ifdef _WIN32
    if (SLIC3RAPP) SLIC3RAPP->MSWForceDarkMode(this->dark_mode);
#endif
    
    // Refresh all top-level windows if they exist
     for (wxWindow* win : wxTopLevelWindows) {
        if (win) {
            this->apply_theme_to_window(win);
            #ifdef _WIN32
             // Re-apply title bar color specifically for this window as iterating wxTopLevelWindows in App::MSWForceDarkMode might miss new ones
             if(SLIC3RAPP) SLIC3RAPP->MSWForceDarkMode(this->dark_mode);
            #endif
        }
     }
}

void Settings::apply_theme_to_window(wxWindow* win) {
     if (!win) return;
     
     auto theme = ThemeManager::GetColors();
     wxColour target_bg = theme.bg;
     
     win->SetBackgroundColour(target_bg);
     win->Refresh();
     
     // Special handling for MainFrame to update its custom panels
     if (MainFrame* main = dynamic_cast<MainFrame*>(win)) {
         main->sync_colors();
     }

     // Aggressively recurse children to force background color AND text color
     // This fixes issues where panels inside dialogs stick to System Dark Mode colors
     std::function<void(wxWindow*)> recurse_children = [&](wxWindow* parent) {
         for (wxWindow* child : parent->GetChildren()) {
              // Skip simple buttons or text controls, mainly target Panels/Notebooks
              // casting to wxPanel check is good heuristic for containers
              if (dynamic_cast<wxPanel*>(child) || dynamic_cast<wxNotebook*>(child) || dynamic_cast<wxSimplebook*>(child)) {
                  child->SetBackgroundColour(target_bg);
              }
              
              // Fix Controls in Light Mode (force White BG / Black Text)
              if (!ThemeManager::IsDark()) {
                  // Text Inputs, Combo Boxes - Force White BG, Black Text
                  if (dynamic_cast<wxTextCtrl*>(child) || dynamic_cast<wxChoice*>(child) || dynamic_cast<wxComboBox*>(child)) {
                      child->SetBackgroundColour(*wxWHITE);
                      child->SetForegroundColour(*wxBLACK);
                  }
                  // Static Text, Checkboxes - Force Transparent/Target BG, Black Text
                  if (dynamic_cast<wxStaticText*>(child) || dynamic_cast<wxCheckBox*>(child) || dynamic_cast<wxRadioButton*>(child)) {
                       child->SetForegroundColour(*wxBLACK);
                  }
              } else {
                  // Dark Mode Handling
                  if (dynamic_cast<wxCheckBox*>(child) || dynamic_cast<wxRadioButton*>(child)) {
                       // Allow background to be transparent/inherited (which matches the panel's dark bg)
                       // Force text to white for visibility
                       child->SetForegroundColour(*wxWHITE); 
                  }
                  else if (dynamic_cast<wxStaticText*>(child)) {
                       child->SetForegroundColour(*wxWHITE); // Keep text white
                  }
              }
              
              child->Refresh();
              recurse_children(child);
         }
     };
     recurse_children(win);
}

}} // namespace Slic3r::GUI
