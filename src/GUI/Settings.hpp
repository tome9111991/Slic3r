#ifndef SETTINGS_HPP
#define SETTINGS_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <map>
#include <memory>
#include <tuple>
#include <vector>
#include <array>
#include <algorithm>

#include "libslic3r.h"
#include "Theme/ThemeManager.hpp"
#include "Theme/CanvasTheme.hpp"
#include "Preset.hpp"

namespace Slic3r { namespace GUI {

enum class PathColor {
    role
};

enum class ReloadBehavior {
    all, copy, discard
};

/// Stub class to hold onto GUI-specific settings options. 
/// TODO: Incorporate the system from libslic3r
class Settings { 
    public:
        bool show_host {true};
        bool version_check {true};
        bool autocenter {true};
        bool autoalignz {true};
        bool invert_zoom {false};
        bool background_processing {false};
        bool remember_output_path {true};

        bool preset_editor_tabs {true};

        bool hide_reload_dialog {false};
        bool reload_preserve_trafo {false};

        bool dark_mode {true}; // Default to true as its most popular now

        ReloadBehavior reload {ReloadBehavior::all};
        // std::unique_ptr<ColorScheme> color; // REMOVED: Replaced by CanvasTheme
        PathColor color_toolpaths_by {PathColor::role};

        float nudge {1.0}; //< 2D plater nudge amount in mm

        unsigned int threads {1}; //< Number of threads to use when slicing
        
        std::string rotation_controls {"xyz"};

        const wxString version { wxString(SLIC3R_VERSION) };

        wxString skein_directory {}; //< Recently-opened skien directory.

        void save_settings();
        void load_settings();
        void apply_theme();
        void apply_theme_to_window(wxWindow* win);

        std::array<std::vector<wxString>, preset_types> default_presets {};

        // Quick Settings (Pinned)
        std::vector<std::string> quick_settings {};
        bool is_quick_setting(const std::string& key) const {
             return std::find(quick_settings.begin(), quick_settings.end(), key) != quick_settings.end();
        }
        void toggle_quick_setting(const std::string& key) {
             auto it = std::find(quick_settings.begin(), quick_settings.end(), key);
             if (it == quick_settings.end()) quick_settings.push_back(key);
             else quick_settings.erase(it);
             save_settings();
        }

        /// Storage for window positions
        std::map<wxString, std::tuple<wxPoint, wxSize, bool> > window_pos { std::map<wxString, std::tuple<wxPoint, wxSize, bool> >() };

        void save_window_pos(wxWindow* ref, wxString name);
        void restore_window_pos(wxWindow* ref, wxString name);


        const wxFont& small_font() { return _small_font;}
        const wxFont& small_bold_font() { return _small_bold_font;}
        const wxFont& medium_font() { return _medium_font;}
        const int& scroll_step() { return _scroll_step; }

        static std::unique_ptr<Settings> init_settings() {
            return std::make_unique<Settings>();
        }
        Settings(Settings&&) = default;
        Settings& operator=(Settings&&) = default;
        
        Settings();
    private:
        Settings& operator=(const Settings&) = default;
        Settings(const Settings&) = default;

        const std::string LogChannel {"GUI_Settings"}; //< Which log these messages should go to.

        /// Fonts used by the UI.
        wxFont _small_font;
        wxFont _small_bold_font;
        wxFont _medium_font;

    int _scroll_step {0};
};

extern std::unique_ptr<Settings> ui_settings;

}} //namespace Slic3r::GUI

#endif // SETTINGS_HPP
