#include "Preferences.hpp"
#include "GUI.hpp"
#include "Settings.hpp"
#include "libslic3r/Config.hpp"
#include "libslic3r/PrintConfig.hpp"
#include <wx/statbox.h>
#include <boost/any.hpp>

namespace Slic3r { namespace GUI {

PreferencesDialog::PreferencesDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Preferences"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // Apply Theme Colors
    if (ui_settings && ui_settings->color->SOLID_BACKGROUNDCOLOR()) {
        this->SetBackgroundColour(ui_settings->color->BACKGROUND_COLOR());
        this->SetForegroundColour(*wxWHITE);
    } else {
        this->SetBackgroundColour(wxColour(240, 240, 240));
        this->SetForegroundColour(*wxBLACK);
    }

    auto sizer = new wxBoxSizer(wxVERTICAL);
    
    // Create a static box to hold the options
    auto sb = new wxStaticBox(this, wxID_ANY, _("General"));
    auto sb_sizer = new wxStaticBoxSizer(sb, wxVERTICAL);

    optgroup = new OptionsGroup(this);
    optgroup->set_sizer(sb_sizer);

    // Version Check
    {
        Line line(_("Check for updates"), _("If this is enabled, Slic3r will check for updates daily and display a reminder if a newer version is available."));
        Option opt("version_check", coBool, ConfigOptionBool(true));
        opt.desc.label = _("Check for updates");
        line.append_option(opt);
        optgroup->append_line(line);
    }

    // Remember Output Path
    {
        Line line(_("Remember output directory"), _("If this is enabled, Slic3r will prompt the last output directory instead of the one containing the input files."));
        Option opt("remember_output_path", coBool, ConfigOptionBool(true));
        opt.desc.label = _("Remember output directory");
        line.append_option(opt);
        optgroup->append_line(line);
    }
    
    // Auto-center
    {
        Line line(_("Auto-center parts (x,y)"), _("If this is enabled, Slic3r will auto-center objects around the print bed center."));
        Option opt("autocenter", coBool, ConfigOptionBool(true));
        opt.desc.label = _("Auto-center parts (x,y)");
        line.append_option(opt);
        optgroup->append_line(line);
    }

    // Auto-align Z
    {
        Line line(_("Auto-align parts (z=0)"), _("If this is enabled, Slic3r will auto-align objects z value to be on the print bed at z=0."));
        Option opt("autoalignz", coBool, ConfigOptionBool(true));
        opt.desc.label = _("Auto-align parts (z=0)");
        line.append_option(opt);
        optgroup->append_line(line);
    }

    // Invert Zoom
    {
        Line line(_("Invert zoom in previews"), _("If this is enabled, Slic3r will invert the direction of mouse-wheel zoom in preview panes."));
        Option opt("invert_zoom", coBool, ConfigOptionBool(false));
        opt.desc.label = _("Invert zoom in previews");
        line.append_option(opt);
        optgroup->append_line(line);
    }

     // Background Processing
    {
        Line line(_("Background processing"), _("If this is enabled, Slic3r will pre-process objects as soon as they're loaded in order to save time when exporting G-code."));
        Option opt("background_processing", coBool, ConfigOptionBool(false));
        opt.desc.label = _("Background processing");
        line.append_option(opt);
        optgroup->append_line(line);
    }

    // Threads
    {
        Line line(_("Threads"), _("Number of threads to use for slicing."));
        Option opt("threads", coInt, ConfigOptionInt(1));
        opt.desc.label = _("Threads");
        opt.desc.min = 1;
        opt.desc.max = 256;
        line.append_option(opt);
        optgroup->append_line(line);
    }

    // Tabbed Preset Editors
    {
        Line line(_("Display profile editors as tabs"), _("When opening a profile editor, it will be shown in a dialog or in a tab according to this option."));
        Option opt("tabbed_preset_editors", coBool, ConfigOptionBool(true));
        opt.desc.label = _("Display profile editors as tabs");
        line.append_option(opt);
        optgroup->append_line(line);
    }

    // Show Host
    {
        Line line(_("Show Controller Tab (requires restart)"), _("Shows/Hides the Controller Tab. (Restart of Slic3r required.)"));
        Option opt("show_host", coBool, ConfigOptionBool(true));
        opt.desc.label = _("Show Controller Tab");
        line.append_option(opt);
        optgroup->append_line(line);
    }

    // Hide Dialog on Reload
    {
        Line line(_("Hide Dialog on Reload"), _("When checked, the dialog on reloading files with added parts & modifiers is suppressed."));
        Option opt("reload_hide_dialog", coBool, ConfigOptionBool(false));
        opt.desc.label = _("Hide Dialog on Reload");
        line.append_option(opt);
        optgroup->append_line(line);
    }

    // Keep Transformations on Reload
    {
        Line line(_("Keep Transformations on Reload"), _("When checked, the 'Reload from disk' function tries to preserve the current orientation of the object on the bed."));
        Option opt("reload_preserve_trafo", coBool, ConfigOptionBool(false));
        opt.desc.label = _("Keep Transformations on Reload");
        line.append_option(opt);
        optgroup->append_line(line);
    }

    // Default Reload Behavior
    {
        Line line(_("Default Reload Behavior"), _("Choose the default behavior of the 'Reload from disk' function regarding additional parts and modifiers."));
        Option opt("reload_behavior", coEnum, ConfigOptionEnumGeneric());
        opt.desc.label = _("Default Reload Behavior");
        opt.desc.enum_labels.push_back(_("Reload all").ToUTF8().data());
        opt.desc.enum_labels.push_back(_("Reload main, copy added").ToUTF8().data());
        opt.desc.enum_labels.push_back(_("Reload main, discard added").ToUTF8().data());
        opt.desc.enum_values = std::vector<std::string>({ "0", "1", "2" }); 
        line.append_option(opt);
        optgroup->append_line(line);
    }

    // Rotation controls
    {
        Line line(_("Rotation controls in toolbar"), _("What rotation controls to show in the toolbar. (Restart of Slic3r required.)"));
        Option opt("rotation_controls", coEnum, ConfigOptionEnumGeneric());
        opt.desc.label = _("Rotation controls in toolbar");
        opt.desc.enum_labels = std::vector<std::string>({ "Z only", "X,Y,Z", "X,Y,Z (big buttons)" });
        opt.desc.enum_values = std::vector<std::string>({ "z", "xyz", "xyz-big" }); 
        line.append_option(opt);
        optgroup->append_line(line);
    }
    
    // Dark Mode
    {
        Line line(_("Enable Dark Mode"), _("If this is enabled, Slic3r will use a dark color scheme."));
        Option opt("dark_mode", coBool, ConfigOptionBool(true));
        opt.desc.label = _("Enable Dark Mode");
        line.append_option(opt);
        optgroup->append_line(line);
    }
    
    // Set initial values from App Settings
    if (SLIC3RAPP && SLIC3RAPP->settings()) {
        auto settings = SLIC3RAPP->settings();
        
        // Create a temporary definition for these app-specific options
        ConfigDef app_def;
        app_def.add("version_check", coBool);
        app_def.add("remember_output_path", coBool);
        app_def.add("autocenter", coBool);
        app_def.add("autoalignz", coBool);
        app_def.add("invert_zoom", coBool);
        app_def.add("background_processing", coBool);
        app_def.add("threads", coInt);
        app_def.add("tabbed_preset_editors", coBool);
        app_def.add("show_host", coBool);
        app_def.add("reload_hide_dialog", coBool);
        app_def.add("reload_preserve_trafo", coBool);
        app_def.add("dark_mode", coBool);
        
        // Define enums in app_def so DynamicConfig creates the right objects
        {
            ConfigOptionDef def;
            def.type = coEnum;
            def.enum_values = std::vector<std::string>({ "0", "1", "2" });
            def.enum_keys_map["0"] = 0;
            def.enum_keys_map["1"] = 1;
            def.enum_keys_map["2"] = 2;
            app_def.add("reload_behavior", def);
        }
        {
            ConfigOptionDef def;
            def.type = coEnum;
            def.enum_values = std::vector<std::string>({ "z", "xyz", "xyz-big" });
            def.enum_keys_map["z"] = 0;
            def.enum_keys_map["xyz"] = 1;
            def.enum_keys_map["xyz-big"] = 2;
            app_def.add("rotation_controls", def);
        }

        // Populate a temporary config
        DynamicConfig temp_config(&app_def);
        temp_config.setBool("version_check", settings->version_check);
        temp_config.setBool("remember_output_path", settings->remember_output_path);
        temp_config.setBool("autocenter", settings->autocenter);
        temp_config.setBool("autoalignz", settings->autoalignz);
        temp_config.setBool("invert_zoom", settings->invert_zoom);
        temp_config.setBool("background_processing", settings->background_processing);
        temp_config.setInt("threads", (int)settings->threads);
        temp_config.setBool("tabbed_preset_editors", settings->preset_editor_tabs);
        temp_config.setBool("show_host", settings->show_host);
        temp_config.setBool("reload_hide_dialog", settings->hide_reload_dialog);
        temp_config.setBool("reload_preserve_trafo", settings->reload_preserve_trafo);
        temp_config.setBool("dark_mode", settings->dark_mode);
        
        // Handling enums (stored as indices or strings in Settings)
        // Settings stores ReloadBehavior enum, but UI uses int indices for this select
        if (auto opt = temp_config.opt<ConfigOptionInt>("reload_behavior", true)) {
            opt->value = (int)settings->reload;
        }

        // Rotation controls (stored as string in Settings)
        std::string rot = settings->rotation_controls;
        int rot_idx = 0;
        if (rot == "xyz") rot_idx = 1;
        else if (rot == "xyz-big") rot_idx = 2;
        
        if (auto opt = temp_config.opt<ConfigOptionInt>("rotation_controls", true)) {
            opt->value = rot_idx;
        }
        
        optgroup->update_options(&temp_config);
    }

    optgroup->on_change = [this](const std::string& key, boost::any value) {
        if (key == "dark_mode") {
            try {
                bool val = boost::any_cast<bool>(value);
                if (SLIC3RAPP && SLIC3RAPP->settings()) {
                    auto settings = SLIC3RAPP->settings();
                    if (settings->dark_mode != val) {
                        settings->dark_mode = val;
                        settings->apply_theme();
                        this->Refresh(); // Refresh the dialog itself too
                    }
                }
            } catch (...) {}
        }
    };

    sizer->Add(sb_sizer, 1, wxEXPAND | wxALL, 10);

    auto buttons = this->CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    this->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { this->_accept(); }, wxID_OK);
    
    sizer->Add(buttons, 0, wxEXPAND | wxALL, 10);

    this->SetSizer(sizer);
    sizer->SetSizeHints(this);

    // Apply theme on Show to ensure controls are realized and can be styled correclty
    this->Bind(wxEVT_SHOW, [this](wxShowEvent& e) {
        if (e.IsShown() && ui_settings) {
             ui_settings->apply_theme_to_window(this);
        }
        e.Skip();
    });
}

void PreferencesDialog::_accept() {
    if (!SLIC3RAPP) {
        EndModal(wxID_CANCEL);
        return;
    }
    auto settings = SLIC3RAPP->settings();
    bool restart_needed = false;
    
    // Read values
    if (auto f = optgroup->get_ui_field("version_check"))
        settings->version_check = f->get_bool();

    if (auto f = optgroup->get_ui_field("remember_output_path"))
        settings->remember_output_path = f->get_bool();

    if (auto f = optgroup->get_ui_field("autocenter"))
        settings->autocenter = f->get_bool();

    if (auto f = optgroup->get_ui_field("autoalignz"))
        settings->autoalignz = f->get_bool();

    if (auto f = optgroup->get_ui_field("invert_zoom"))
        settings->invert_zoom = f->get_bool();
    
    if (auto f = optgroup->get_ui_field("background_processing"))
        settings->background_processing = f->get_bool();

    if (auto f = optgroup->get_ui_field("threads"))
        settings->threads = (unsigned int)f->get_int();

    if (auto f = optgroup->get_ui_field("tabbed_preset_editors"))
        settings->preset_editor_tabs = f->get_bool();

    if (auto f = optgroup->get_ui_field("show_host")) {
        bool val = f->get_bool();
        if (settings->show_host != val) restart_needed = true;
        settings->show_host = val;
    }

    if (auto f = optgroup->get_ui_field("reload_hide_dialog"))
        settings->hide_reload_dialog = f->get_bool();

    if (auto f = optgroup->get_ui_field("reload_preserve_trafo"))
        settings->reload_preserve_trafo = f->get_bool();

    if (auto f = optgroup->get_ui_field("dark_mode")) {
        bool val = f->get_bool();
        if (settings->dark_mode != val) {
            settings->dark_mode = val;
            settings->apply_theme();
        }
    }

    if (auto f = optgroup->get_ui_field("reload_behavior"))
        settings->reload = (ReloadBehavior)f->get_int();

    if (auto f = optgroup->get_ui_field("rotation_controls")) {
        int val = f->get_int();
        std::string str_val = "z";
        if (val == 1) str_val = "xyz";
        if (val == 2) str_val = "xyz-big";
        
        if (settings->rotation_controls != str_val) restart_needed = true;
        settings->rotation_controls = str_val;
    }

    // Save to disk
    settings->save_settings();
    
    if (restart_needed) {
        wxMessageBox(_("You need to restart Slic3r to make the changes effective."), _("Slic3r"), wxICON_INFORMATION);
    }
    
    EndModal(wxID_OK);
}

}} // namespace Slic3r::GUI
