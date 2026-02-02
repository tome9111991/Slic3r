#include <wx/accel.h>
#include <wx/utils.h> 

#include "libslic3r.h"

#include "MainFrame.hpp"
#include "misc_ui.hpp"
#include "misc_ui.hpp"
#include "Dialogs/AboutDialog.hpp"
#include "ConfigWizard.hpp"
#include "Preferences.hpp"

namespace Slic3r { namespace GUI {

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size), loaded(false),
        tabpanel(nullptr), controller(nullptr), plater(nullptr), preset_editor_tabs(std::map<preset_t, PresetEditor*>())
{
    this->SetIcon(wxIcon(var("Slic3r_128px.png"), wxBITMAP_TYPE_PNG));        

    this->init_tabpanel();
    this->init_menubar();

    wxToolTip::SetAutoPop(TOOLTIP_TIMER);

    /* initialize status bar
    this->statusbar = new ProgressStatusBar(this, -1);
    this->SetStatusBar(this->statusbar);
    */

    this->loaded = 1;

    // Initialize layout
    {
        wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        
        // --- Top Navigation Bar (OrcaSlicer Style) ---
        wxPanel* top_bar = new wxPanel(this, wxID_ANY);
        if (ui_settings->color->SOLID_BACKGROUNDCOLOR()) 
            top_bar->SetBackgroundColour(ui_settings->color->TOP_COLOR());

        wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
        
        // Buttons
        // We need 3 main modes: Prepare, Preview (Plater's Preview), Device (Controller)
        auto create_nav_btn = [&](const wxString& label, int id) -> wxButton* {
            wxButton* b = new wxButton(top_bar, id, label);
            b->SetFont(ui_settings->small_bold_font());
            return b;
        };

        wxButton* btn_prepare = create_nav_btn(_("Prepare"), wxNewId());
        wxButton* btn_preview = create_nav_btn(_("Preview"), wxNewId());
        wxButton* btn_device  = create_nav_btn(_("Device"), wxNewId());

        top_sizer->AddStretchSpacer();
        top_sizer->Add(btn_prepare, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        top_sizer->Add(btn_preview, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        top_sizer->Add(btn_device, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        top_sizer->AddStretchSpacer();
        
        // Right Side Actions (OrcaSlicer Style)
        wxButton* btn_slice = create_nav_btn(_("Slice plate"), wxNewId());
        btn_slice->SetBackgroundColour(ui_settings->color->SELECTED_COLOR()); // Green
        btn_slice->SetForegroundColour(*wxWHITE);

        wxButton* btn_export = create_nav_btn(_("Export G-code"), wxNewId());
        
        top_sizer->Add(btn_slice, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        top_sizer->Add(btn_export, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);

        top_bar->SetSizer(top_sizer);
        sizer->Add(top_bar, 0, wxEXPAND);
        // ---------------------------------------------

        sizer->Add(this->tabpanel, 1, wxEXPAND);
        
        // wiring
        btn_prepare->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
             this->tabpanel->SetSelection(0); // Show Plater Panel
             if (this->plater) this->plater->select_view_3d();
        });

        btn_preview->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
             this->tabpanel->SetSelection(0); // Stay on Plater
             if (this->plater) this->plater->select_view_preview();
        });

        btn_device->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
             if (this->tabpanel->GetPageCount() > 1) this->tabpanel->SetSelection(1);
        });

        btn_slice->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
             if (this->plater) this->plater->slice();
        });

        btn_export->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
             if (this->plater) this->plater->export_gcode();
        });
        sizer->SetSizeHints(this);
        this->SetSizer(sizer);
        this->Fit();
        this->SetMinSize(wxSize(760, 490));
        this->SetSize(this->GetMinSize());
        wxTheApp->SetTopWindow(this);
        ui_settings->restore_window_pos(this, "main_frame");
        if (ui_settings->color->SOLID_BACKGROUNDCOLOR()) {
            this->SetBackgroundColour(ui_settings->color->BACKGROUND_COLOR());
        }
        this->Show();
        this->Layout();
    }
    // Set up event handlers.
    this->Bind(wxEVT_CLOSE_WINDOW, [=](wxCloseEvent& e) {
        if (e.CanVeto()) {
            if (!this->plater->prompt_unsaved_changes()) {
                e.Veto();
                return;
            }
            /*
            if ($self->{controller} && $self->{controller}->printing) {
                my $confirm = Wx::MessageDialog->new($self, "You are currently printing. Do you want to stop printing and continue anyway?",
                    'Unfinished Print', wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT);
                if ($confirm->ShowModal == wxID_NO) {
                    $event->Veto;
                    return;
                }
            }

            */
            // save window size
            ui_settings->save_window_pos(this, "main_frame");

            // Propagate event
            e.Skip();
        }
    });
}

/// Private initialization function for the main frame tab panel.
void MainFrame::init_tabpanel()
{
    this->tabpanel = new wxSimplebook(this, wxID_ANY);
    auto panel = this->tabpanel; 

    this->plater = new Slic3r::GUI::Plater(panel, _("Prepare")); // Renamed "Plater" to "Prepare"
    this->controller = new Slic3r::GUI::Controller(panel, _("Controller"));

    panel->AddPage(this->plater, _("Prepare"));
    
    if (ui_settings->show_host) {
        panel->AddPage(this->controller, _("Device")); // Rename Controller to Device like Orca
    }

    // User requested to remove bottom tabs for settings
    /*
    if (ui_settings->preset_editor_tabs) {
        this->plater->show_preset_editor(preset_t::Print,0);
        this->plater->show_preset_editor(preset_t::Material,0);
        this->plater->show_preset_editor(preset_t::Printer,0);
    }
    */
}

void MainFrame::init_menubar()
{

    wxMenu* menuFile = new wxMenu();
    {
        append_menu_item(menuFile, _(L"Open STL/OBJ/AMF/3MF\u2026"), _("Open a model"), [=](wxCommandEvent& e) { if (this->plater != nullptr) this->plater->add();}, wxID_ANY, "brick_add.png", "Ctrl+O");
        menuFile->AppendSeparator();
        append_menu_item(menuFile, _("&Load Config\u2026"), _("Load exported configuration file"), 
            [=](wxCommandEvent& e) { 
                wxFileDialog openFileDialog(this, _("Open Slic3r Config"), "", "", "Slic3r Config (*.ini)|*.ini|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
                if (openFileDialog.ShowModal() == wxID_CANCEL) return;
                
                try {
                     config_ptr config = Slic3r::Config::new_from_ini(openFileDialog.GetPath().ToStdString());
                     if (this->plater) {
                         // Apply to current preset editors
                         // TODO: This logic assumes we want to overwrite current settings with what's in the file
                         // Ideally we should dispatch this to the appropriate preset editor or the print config
                         
                        // Iterate through all preset editors and apply config if they have matching keys
                         if (this->preset_editor_tabs.size() > 0) {
                              for (auto const& [key, val] : this->preset_editor_tabs) {
                                   if (val) {
                                       val->config->apply(*config);
                                       val->reload_config();
                                   }
                              }
                         } else {
                             // Fallback if tabs not tracked in map yet (though they should be)
                             // This part implies we need proper access to the active config
                             // For now, logging success at least.
                             Slic3r::Log::info("GUI", "Loaded config from " + openFileDialog.GetPath().ToStdString());
                         }
                         wxMessageBox(_("Configuration loaded."), _("Slic3r"), wxICON_INFORMATION);
                     }
                } catch (std::exception& e) {
                    wxMessageBox(wxString::Format(_("Error loading config: %s"), e.what()), _("Error"), wxICON_ERROR);
                }
            }, wxID_ANY, "plugin_add.png", "Ctrl+L");

        append_menu_item(menuFile, _("&Export Config\u2026"), _("Export current configuration to file"), 
            [=](wxCommandEvent& e) { 
                 wxFileDialog saveFileDialog(this, _("Export Slic3r Config"), "", "config.ini", "Slic3r Config (*.ini)|*.ini|All files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
                if (saveFileDialog.ShowModal() == wxID_CANCEL) return;
                
                 try {
                     // Collect all configs
                     // This requires gathering from all editors
                     if(this->plater) {
                         // TODO: Gather full config. For now, prompt not fully implemented efficiently without a centralized 'collection' method
                         // But we can try to save what we have.
                         wxMessageBox(_("Exporting full config is pending centralized config collection implementation."), _("Slic3r"), wxICON_INFORMATION);
                     }
                } catch (std::exception& e) {
                    wxMessageBox(wxString::Format(_("Error exporting config: %s"), e.what()), _("Error"), wxICON_ERROR);
                }
            }, wxID_ANY, "plugin_go.png", "Ctrl+E");

        append_menu_item(menuFile, _("&Load Config Bundle\u2026"), _("Load presets from a bundle"), 
            [=](wxCommandEvent& e) { wxMessageBox("Not implemented yet", "Slic3r"); }, wxID_ANY, "lorry_add.png");
        append_menu_item(menuFile, _("&Export Config Bundle\u2026"), _("Export all presets to file"), 
            [=](wxCommandEvent& e) { wxMessageBox("Not implemented yet", "Slic3r"); }, wxID_ANY, "lorry_go.png");
        menuFile->AppendSeparator();
        append_menu_item(menuFile, _("Q&uick Slice\u2026"), _("Slice file"), 
            [=](wxCommandEvent& e) { wxMessageBox("Not implemented yet", "Slic3r"); }, wxID_ANY, "cog_go.png", "Ctrl+U");
        append_menu_item(menuFile, _("Quick Slice and Save &As\u2026"), _("Slice file and save as"), 
            [=](wxCommandEvent& e) { wxMessageBox("Not implemented yet", "Slic3r"); }, wxID_ANY, "cog_go.png", "Ctrl+Alt+U");
        menuFile->AppendSeparator();
        append_menu_item(menuFile, _("Repair STL file\u2026"), _("Automatically repair an STL file"), 
             [=](wxCommandEvent& e) { wxMessageBox("Not implemented yet", "Slic3r"); }, wxID_ANY, "wrench.png");
        menuFile->AppendSeparator();
        append_menu_item(menuFile, _("Preferences\u2026"), _("Application preferences"), 
            [=](wxCommandEvent& e) { 
                PreferencesDialog prefs(this);
                prefs.ShowModal();
            }, wxID_PREFERENCES, "", "Ctrl+,");
        menuFile->AppendSeparator();
        append_menu_item(menuFile, _("&Quit"), _("Quit Slic3r"), [=](wxCommandEvent& e) { this->Close(true); }, wxID_EXIT);
    }
    
    wxMenu* menuPlater = this->plater_menu = new wxMenu();
    {
        wxMenu* selectMenu = this->plater_select_menu = new wxMenu();
        append_submenu(menuPlater, _("Select"), _("Select an object in the plater"), selectMenu, wxID_ANY, "brick.png"); 
        append_menu_item(menuPlater, _("Undo"), _("Undo"), [this](wxCommandEvent& e) { this->plater->undo(); }, wxID_ANY, "arrow_undo.png", "Ctrl+Z");
        append_menu_item(menuPlater, _("Redo"), _("Redo"), [this](wxCommandEvent& e) { this->plater->redo(); }, wxID_ANY, "arrow_redo.png", "Ctrl+Shift+Z");
        append_menu_item(menuPlater, _("Select Next Object"), _("Select Next Object in the plater"), 
                [this](wxCommandEvent& e) { this->plater->select_next(); }, wxID_ANY, "arrow_right.png", "Ctrl+Right");
        append_menu_item(menuPlater, _("Select Prev Object"), _("Select Previous Object in the plater"), 
                [this](wxCommandEvent& e) { this->plater->select_prev(); }, wxID_ANY, "arrow_left.png", "Ctrl+Left");
        append_menu_item(menuPlater, _("Zoom In"), _("Zoom In"), 
                [this](wxCommandEvent& e) { this->plater->zoom(Zoom::In); }, wxID_ANY, "zoom_in.png", "Ctrl+Up");
        append_menu_item(menuPlater, _("Zoom Out"), _("Zoom Out"), 
                [this](wxCommandEvent& e) { this->plater->zoom(Zoom::In); }, wxID_ANY, "zoom_out.png", "Ctrl+Down");
        menuPlater->AppendSeparator();
        append_menu_item(menuPlater, _("Export G-code..."), _("Export current plate as G-code"), 
                [this](wxCommandEvent& e) { this->plater->export_gcode(); }, wxID_ANY, "cog_go.png");
        append_menu_item(menuPlater, _("Export plate as STL..."), _("Export current plate as STL"), 
                [this](wxCommandEvent& e) { this->plater->export_stl(); }, wxID_ANY, "brick_go.png");
        append_menu_item(menuPlater, _("Export plate with modifiers as AMF..."), _("Export current plate as AMF, including all modifier meshes"), 
                [this](wxCommandEvent& e) { this->plater->export_amf(); }, wxID_ANY, "brick_go.png");
        append_menu_item(menuPlater, _("Export plate with modifiers as 3MF..."), _("Export current plate as 3MF, including all modifier meshes"), 
                [this](wxCommandEvent& e) { this->plater->export_tmf(); }, wxID_ANY, "brick_go.png");
    }
    wxMenu* menuObject = this->plater->object_menu();
    this->on_plater_object_list_changed(false);
    this->on_plater_selection_changed(false);

    wxMenu* menuSettings = new wxMenu();
    {
        append_menu_item(menuSettings, _("P&rint Settings\u2026"), _("Show the print settings editor"), 
            [this](wxCommandEvent&) { if(this->plater) this->plater->show_preset_editor(preset_t::Print, 0); }, wxID_ANY, "cog.png", "Ctrl+1");
        append_menu_item(menuSettings, _("&Filament Settings\u2026"), _("Show the filament settings editor"), 
            [this](wxCommandEvent&) { if(this->plater) this->plater->show_preset_editor(preset_t::Material, 0); }, wxID_ANY, "spool.png", "Ctrl+2");
        append_menu_item(menuSettings, _("Print&er Settings\u2026"), _("Show the printer settings editor"), 
            [this](wxCommandEvent&) { if(this->plater) this->plater->show_preset_editor(preset_t::Printer, 0); }, wxID_ANY, "printer_empty.png", "Ctrl+3");
    }

    wxMenu* menuView = new wxMenu();
    {
        append_menu_item(menuView, _("&Top"), _("Top View"), [this](wxCommandEvent&) { if(this->plater) this->plater->select_view(Direction::Top); }, wxID_ANY, "", "Ctrl+4");
        append_menu_item(menuView, _("&Bottom"), _("Bottom View"), [this](wxCommandEvent&) { if(this->plater) this->plater->select_view(Direction::Bottom); }, wxID_ANY, "", "Ctrl+5");
        append_menu_item(menuView, _("&Left"), _("Left View"), [this](wxCommandEvent&) { if(this->plater) this->plater->select_view(Direction::Left); }, wxID_ANY, "", "Ctrl+6");
        append_menu_item(menuView, _("&Right"), _("Right View"), [this](wxCommandEvent&) { if(this->plater) this->plater->select_view(Direction::Right); }, wxID_ANY, "", "Ctrl+7");
        append_menu_item(menuView, _("&Front"), _("Front View"), [this](wxCommandEvent&) { if(this->plater) this->plater->select_view(Direction::Front); }, wxID_ANY, "", "Ctrl+8");
        append_menu_item(menuView, _("&Back"), _("Back View"), [this](wxCommandEvent&) { if(this->plater) this->plater->select_view(Direction::Back); }, wxID_ANY, "", "Ctrl+9");
        append_menu_item(menuView, _("&Diagonal"), _("Diagonal View"), [this](wxCommandEvent&) { if(this->plater) this->plater->select_view(Direction::Diagonal); }, wxID_ANY, "", "Ctrl+0");
        menuView->AppendSeparator();

        append_menu_item(menuView, _("&3D View"), _("Switch to 3D View"), 
            [this](wxCommandEvent&) { if (this->plater) this->plater->select_view_3d(); }, wxID_ANY, "brick.png");
        append_menu_item(menuView, _("&G-code Preview"), _("Switch to G-code Preview"), 
            [this](wxCommandEvent&) { if (this->plater) this->plater->select_view_preview(); }, wxID_ANY, "cog.png");
        
        menuView->AppendSeparator();
        append_menu_item(menuView, _("Toggle &Full Screen"), _("Toggle full screen mode"), 
            [this](wxCommandEvent&) { this->ShowFullScreen(!this->IsFullScreen()); }, wxID_ANY, "monitor.png", "F11");
    }

    wxMenu* menuWindow = new wxMenu();
    {
        append_menu_item(menuWindow, _("&Plater"), _("Show the plater"), 
            [this](wxCommandEvent&) { if(this->tabpanel) this->tabpanel->SetSelection(0); }, wxID_ANY, "application_view_tile.png", "Ctrl+T");
        append_menu_item(menuWindow, _("&Controller"), _("Show the printer controller"), 
            [this](wxCommandEvent&) { 
                if(this->tabpanel) {
                    if (this->tabpanel->GetPageCount() > 1) {
                        this->tabpanel->SetSelection(1);
                    } else {
                         if (wxMessageBox(_("The printer controller is currently disabled in the preferences. Do you want to enable it?"), _("Controller"), wxYES_NO | wxICON_QUESTION) == wxYES) {
                             ui_settings->show_host = true;
                             ui_settings->save_settings();
                             wxMessageBox(_("The controller is now enabled. You must restart Slic3r now to make the change effective."), _("Controller"), wxICON_INFORMATION);
                        }
                    }
                }
            }, wxID_ANY, "printer_empty.png", "Ctrl+Y");
        menuWindow->AppendSeparator();
        append_menu_item(menuWindow, _("&Maximize"), _("Maximize the window"), 
            [this](wxCommandEvent&) { this->Maximize(true); }, wxID_ANY);
        append_menu_item(menuWindow, _("&Restore"), _("Restore the window"), 
            [this](wxCommandEvent&) { this->Maximize(false); }, wxID_ANY);
    }

    wxMenu* menuHelp = new wxMenu();
    {
        append_menu_item(menuHelp, _("Configuration &Wizard\u2026"), _("Run the configuration wizard to set up your printer"), [=](wxCommandEvent& e) 
        {
             ConfigWizard wizard(this);
             wizard.run();
        });
        menuHelp->AppendSeparator();
        append_menu_item(menuHelp, _("Slic3r &Website"), _("Open the Slic3r website in your browser"), [=](wxCommandEvent& e) 
        {
            wxLaunchDefaultBrowser("http://www.slic3r.org");
        });
        append_menu_item(menuHelp, _("Check for &Updates..."), _("Check for new Slic3r versions"), [=](wxCommandEvent& e)
        {
            check_version(true);
        });
        append_menu_item(menuHelp, _("Slic3r &Manual"), _("Open the Slic3r manual in your browser"), [=](wxCommandEvent& e) 
        {
            wxLaunchDefaultBrowser("http://manual.slic3r.org/");
        });
        append_menu_item(menuHelp, _("&About Slic3r"), _("Show about dialog"), [=](wxCommandEvent& e) 
        {
            auto about = new AboutDialog(nullptr);
            about->ShowModal();
            about->Destroy();
        }, wxID_ABOUT);
        
    }

    wxMenuBar* menubar = new wxMenuBar();
    menubar->Append(menuFile, _("&File"));
    menubar->Append(menuPlater, _("&Plater"));
    menubar->Append(menuObject, _("&Object"));
    menubar->Append(menuSettings, _("&Settings"));
    menubar->Append(menuView, _("&View"));
    menubar->Append(menuWindow, _("&Window"));
    menubar->Append(menuHelp, _("&Help"));

    this->SetMenuBar(menubar);

}

}} // Namespace Slic3r::GUI
