#include "Preferences.hpp"
#include "GUI.hpp"
#include "Settings.hpp"
#include "libslic3r/Config.hpp"
#include "libslic3r/PrintConfig.hpp"
#include <wx/statbox.h>

namespace Slic3r { namespace GUI {

PreferencesDialog::PreferencesDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Preferences"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    auto sizer = new wxBoxSizer(wxVERTICAL);
    
    // Create a static box to hold the options
    auto sb = new wxStaticBox(this, wxID_ANY, _("General"));
    auto sb_sizer = new wxStaticBoxSizer(sb, wxVERTICAL);

    optgroup = new OptionsGroup(this);
    optgroup->set_sizer(sb_sizer);

    // Version Check
    {
        Line line(_("Check for updates"), _("If this is enabled, Slic3r will check for updates daily."));
        Option opt("version_check", coBool, ConfigOptionBool(true));
        opt.desc.label = _("Check for updates");
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

    // Show Host
    {
        Line line(_("Show Controller Tab (requires restart)"), _("Shows/Hides the Controller Tab. (Restart of Slic3r required.)"));
        Option opt("show_host", coBool, ConfigOptionBool(false));
        opt.desc.label = _("Show Controller Tab");
        line.append_option(opt);
        optgroup->append_line(line);
    }
    
     // Background Processing
    {
        Line line(_("Background processing"), _("If this is enabled, Slic3r will pre-process objects as soon as they're loaded."));
        Option opt("background_processing", coBool, ConfigOptionBool(true));
        opt.desc.label = _("Background processing");
        line.append_option(opt);
        optgroup->append_line(line);
    }

    // Set initial values from App Settings
    if (SLIC3RAPP && SLIC3RAPP->settings()) {
        auto settings = SLIC3RAPP->settings();
        
        // Populate a temporary config
        DynamicPrintConfig temp_config;
        temp_config.optptr("version_check", true)->set(ConfigOptionBool(settings->version_check));
        temp_config.optptr("autocenter", true)->set(ConfigOptionBool(settings->autocenter));
        temp_config.optptr("show_host", true)->set(ConfigOptionBool(settings->show_host));
        temp_config.optptr("background_processing", true)->set(ConfigOptionBool(settings->background_processing));
        
        optgroup->update_options(&temp_config);
    }

    sizer->Add(sb_sizer, 1, wxEXPAND | wxALL, 10);

    auto buttons = this->CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    this->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { this->_accept(); }, wxID_OK);
    
    sizer->Add(buttons, 0, wxEXPAND | wxALL, 10);

    this->SetSizer(sizer);
    sizer->SetSizeHints(this);
}

void PreferencesDialog::_accept() {
    if (!SLIC3RAPP) {
        EndModal(wxID_CANCEL);
        return;
    }
    auto settings = SLIC3RAPP->settings();
    
    // Read values
    if (auto f = optgroup->get_ui_field("version_check"))
        settings->version_check = f->get_bool();

    if (auto f = optgroup->get_ui_field("autocenter"))
        settings->autocenter = f->get_bool();
    
    bool old_show_host = settings->show_host;
    if (auto f = optgroup->get_ui_field("show_host"))
        settings->show_host = f->get_bool();
    
    if (auto f = optgroup->get_ui_field("background_processing"))
        settings->background_processing = f->get_bool();

    // Save to disk
    settings->save_settings();
    
    if (old_show_host != settings->show_host) {
        wxMessageBox(_("You need to restart Slic3r to make the changes effective."), _("Slic3r"), wxICON_INFORMATION);
    }
    
    EndModal(wxID_OK);
}

}} // namespace Slic3r::GUI
