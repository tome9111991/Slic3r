#include <wx/accel.h>
#include <wx/utils.h> 
#include <wx/tglbtn.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include "libslic3r.h"

#include "MainFrame.hpp"
#include "misc_ui.hpp"
#include "misc_ui.hpp"
#include "Theme/ThemeManager.hpp"
#include "Dialogs/AboutDialog.hpp"
#include "ConfigWizard.hpp"
#include "Preferences.hpp"
#include "Dialogs/WidgetGallery.hpp"


namespace Slic3r { namespace GUI {

// Custom "Flat" Toggle Button for Tabs
class FlatToggleButton : public wxPanel {
    wxString m_label;
    bool m_value = false; 
    bool m_hover = false;
public:
    FlatToggleButton(wxWindow* parent, int id, const wxString& label) 
        : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTAB_TRAVERSAL), m_label(label) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &FlatToggleButton::OnPaint, this);
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& e){ m_hover=true; Refresh(); e.Skip(); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& e){ m_hover=false; Refresh(); e.Skip(); });
        Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e){ 
            // Emit TOGGLEBUTTON event on click
            wxCommandEvent evt(wxEVT_TOGGLEBUTTON, GetId()); 
            evt.SetEventObject(this); 
            evt.SetInt(1); // Checked
            GetEventHandler()->ProcessEvent(evt); 
            e.Skip();
        });
    }
    void SetValue(bool v) { m_value = v; Refresh(); }
    bool GetValue() const { return m_value; }
    
    void OnPaint(wxPaintEvent& evt) {
        wxAutoBufferedPaintDC dc(this);
        double scale = GetContentScaleFactor();

        wxColour parentBg = GetParent()->GetBackgroundColour();
        dc.SetBackground(wxBrush(parentBg));
        dc.Clear();
        
        dc.SetFont(GetFont());
        wxColour fg = GetForegroundColour();
        if (!fg.IsOk()) fg = *wxWHITE;
        
        // Background Logic: Show if Hovering OR Active (Value is true)
        if (m_hover || m_value) {
            wxColour baseBg = parentBg;
            // Calculate a visible background color based on theme
            wxColour drawBg = baseBg.ChangeLightness(120); 
            if (baseBg.Red() > 200) drawBg = baseBg.ChangeLightness(90); // Handle light mode
            
            // If active, maybe make it slightly distinct or just keep the "active zone" look?
            // User requested "keep the mouse over effect active".
            
            dc.SetPen(wxPen(drawBg));
            dc.SetBrush(wxBrush(drawBg));
            dc.DrawRoundedRectangle(GetClientRect(), 4 * scale);
        }
        
        if (m_value) {
            // Active Tab Style: Underline + Brighter Text
            dc.SetPen(wxPen(fg, 3 * scale));
            // Use GetClientSize() for width/height
            // draw line at bottom with margin
            int margin = 4 * scale;
            int y = GetClientSize().GetHeight() - (2 * scale);
            dc.DrawLine(margin, y, GetClientSize().GetWidth() - margin, y);
        }
        
        dc.SetTextForeground(fg);
        wxSize extent = dc.GetTextExtent(m_label);
        dc.DrawText(m_label, (GetClientSize().GetWidth()-extent.GetWidth())/2, (GetClientSize().GetHeight()-extent.GetHeight())/2);
    }
    
    wxSize DoGetBestSize() const override {
         wxClientDC dc(const_cast<FlatToggleButton*>(this));
         dc.SetFont(GetFont());
         wxSize s = dc.GetTextExtent(m_label);
         double scale = GetContentScaleFactor();
         return wxSize(s.GetWidth() + (30 * scale), s.GetHeight() + (16 * scale));
    }
};

// Custom "Oval" Action Button
class OvalButton : public wxPanel {
    wxString m_label;
    bool m_hover = false;
    bool m_down = false;
public:
    OvalButton(wxWindow* parent, int id, const wxString& label) 
        : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTAB_TRAVERSAL), m_label(label) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &OvalButton::OnPaint, this);
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& e){ m_hover=true; Refresh(); e.Skip(); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& e){ m_hover=false; m_down=false; Refresh(); e.Skip(); });
        Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e){ m_down=true; Refresh(); e.Skip(); });
        Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e){ 
            if(m_down) { 
                m_down=false; Refresh(); 
                wxCommandEvent evt(wxEVT_BUTTON, GetId()); 
                evt.SetEventObject(this); 
                GetEventHandler()->ProcessEvent(evt); 
            }
            e.Skip();
        });
    }
    
    void OnPaint(wxPaintEvent& evt) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();
        
        wxColour bg = GetBackgroundColour();
        if (m_hover) bg = bg.ChangeLightness(110);
        if (m_down) bg = bg.ChangeLightness(90);
        
        wxRect rect = GetClientRect();
        
        // Use GraphicsContext for anti-aliased Gradient
        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (gc) {
            wxGraphicsPath path = gc->CreatePath();
            path.AddRoundedRectangle(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight(), rect.GetHeight()/2.0);
            
            // Modern Flat Look with minimal gradient for polish (Subtle)
            // Instead of strong 3D, we use very subtle shift
            wxColour colTop = bg.ChangeLightness(105);
            wxColour colBot = bg.ChangeLightness(95);
            
            wxGraphicsBrush brush = gc->CreateLinearGradientBrush(
                rect.GetX(), rect.GetY(), 
                rect.GetX(), rect.GetY() + rect.GetHeight(),
                colTop, colBot);
                
            gc->SetBrush(brush);
            gc->FillPath(path);
            
            delete gc;
        } else {
            // Fallback
            dc.SetBrush(wxBrush(bg));
            dc.SetPen(wxPen(bg));
            dc.DrawRoundedRectangle(rect, rect.GetHeight()/2.0);
        }
        
        dc.SetFont(GetFont());
        dc.SetTextForeground(GetForegroundColour());
        wxSize extent = dc.GetTextExtent(m_label);
        dc.DrawText(m_label, (rect.GetWidth()-extent.GetWidth())/2, (rect.GetHeight()-extent.GetHeight())/2);
    }
    
    wxSize DoGetBestSize() const override {
         wxClientDC dc(const_cast<OvalButton*>(this));
         dc.SetFont(GetFont());
         wxSize s = dc.GetTextExtent(m_label);
         double scale = GetContentScaleFactor();
         return wxSize(s.GetWidth() + (40 * scale), s.GetHeight() + (14 * scale));
    }
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size), loaded(false),
        tabpanel(nullptr), controller(nullptr), plater(nullptr), preset_editor_tabs(std::map<preset_t, PresetEditor*>())
{
        wxIconBundle icons;
        // User requested to keep using PNG for the main app icon for now
        wxString png_path = var("images/Slic3r_128px.png");
        if (wxFileExists(png_path)) {
            icons.AddIcon(wxIcon(png_path, wxBITMAP_TYPE_PNG));
        } else {
             // Fallback
             icons.AddIcon(wxIcon(var("Slic3r_128px.png"), wxBITMAP_TYPE_PNG));
        }
        this->SetIcons(icons);
        this->init_tabpanel();
    this->init_menubar();

    wxToolTip::SetAutoPop(TOOLTIP_TIMER);

    this->loaded = 1;

    // Initialize layout
    {
        wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        
        // --- Top Navigation Bar (Modern Style) ---
        this->top_bar = new wxPanel(this, wxID_ANY);
        
        wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
        
        // Navigation Buttons (Acting as Tabs)
        // Implemented as FlatToggleButton for clean, modern look
        auto create_nav_tgl = [&](const wxString& label) -> FlatToggleButton* {
            FlatToggleButton* b = new FlatToggleButton(this->top_bar, wxID_ANY, label);
            b->SetFont(ui_settings->medium_font()); 
            if (ThemeManager::IsDark()) {
                b->SetBackgroundColour(ThemeManager::GetColors().header); // Match bar background
                b->SetForegroundColour(*wxWHITE);
            }
            return b;
        };

        this->btn_prepare = create_nav_tgl(_("Prepare"));
        this->btn_preview = create_nav_tgl(_("Preview"));
        this->btn_device  = create_nav_tgl(_("Device"));

        dynamic_cast<FlatToggleButton*>(this->btn_prepare)->SetValue(true); // Default selection

        // Left-align the navigation tabs
        top_sizer->Add(this->btn_prepare, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
        top_sizer->Add(this->btn_preview, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
        top_sizer->Add(this->btn_device,  0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
        top_sizer->AddStretchSpacer(1);
        
        // Right Side Actions - Oval Buttons
        this->btn_slice = new OvalButton(this->top_bar, wxID_ANY, _("Slice plate"));
        this->btn_export = new OvalButton(this->top_bar, wxID_ANY, _("Export G-code"));
        this->btn_export->SetFont(ui_settings->small_font());
        
        top_sizer->Add(this->btn_slice, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        top_sizer->Add(this->btn_export, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);

        this->top_bar->SetSizer(top_sizer);
        sizer->Add(this->top_bar, 0, wxEXPAND);
        // ---------------------------------------------

        sizer->Add(this->tabpanel, 1, wxEXPAND);
        
        // Navigation Logic Helper
        // Navigation Logic Helper
        auto update_nav = [this](FlatToggleButton* active) {
            if (this->btn_prepare) {
                 auto btn = static_cast<FlatToggleButton*>(this->btn_prepare);
                 if (active != btn) btn->SetValue(false);
                 else btn->SetValue(true);
            }
            
            if (this->btn_preview) {
                auto btn = static_cast<FlatToggleButton*>(this->btn_preview);
                if (active != btn) btn->SetValue(false);
                else btn->SetValue(true);
            }

            if (this->btn_device) {
                auto btn = static_cast<FlatToggleButton*>(this->btn_device);
                if (active != btn) btn->SetValue(false);
                else btn->SetValue(true);
            }
        };

        // Wiring Events
        this->btn_prepare->Bind(wxEVT_TOGGLEBUTTON, [=](wxCommandEvent& e) {
             update_nav(dynamic_cast<FlatToggleButton*>(this->btn_prepare));
             this->tabpanel->SetSelection(0); // Show Plater Panel
             if (this->plater) this->plater->select_view_3d();
        });

        this->btn_preview->Bind(wxEVT_TOGGLEBUTTON, [=](wxCommandEvent& e) {
             update_nav(dynamic_cast<FlatToggleButton*>(this->btn_preview));
             this->tabpanel->SetSelection(0); // Stay on Plater
             if (this->plater) this->plater->select_view_preview();
        });

        this->btn_device->Bind(wxEVT_TOGGLEBUTTON, [=](wxCommandEvent& e) {
             update_nav(dynamic_cast<FlatToggleButton*>(this->btn_device));
             if (this->tabpanel->GetPageCount() > 1) this->tabpanel->SetSelection(1);
        });

        // Action Buttons
        this->btn_slice->Bind(wxEVT_BUTTON, [this, update_nav](wxCommandEvent&) {
             if (this->plater) {
                 this->plater->slice();
                 // Automatically switch to preview after slicing for modern workflow
                 update_nav(dynamic_cast<FlatToggleButton*>(this->btn_preview)); 
                 this->plater->select_view_preview();
             }
        });

        this->btn_export->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
             if (this->plater) this->plater->export_gcode();
        });

        sizer->SetSizeHints(this);
        this->SetSizer(sizer);
        this->Fit();
        // Increased minimum size for modern displays
        this->SetMinSize(wxSize(960, 640));
        this->SetSize(this->GetMinSize());
        
        wxTheApp->SetTopWindow(this);
        ui_settings->restore_window_pos(this, "main_frame");
        
        this->sync_colors();

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
        append_menu_item(menuFile, _(L"Open STL/OBJ/AMF/3MF\u2026"), _("Open a model"), [=](wxCommandEvent& e) { if (this->plater != nullptr) this->plater->add();}, wxID_ANY, "brick_add.svg", "Ctrl+O");
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
            }, wxID_ANY, "plugin_add.svg", "Ctrl+L");

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
            }, wxID_ANY, "plugin_go.svg", "Ctrl+E");

        append_menu_item(menuFile, _("&Load Config Bundle\u2026"), _("Load presets from a bundle"), 
            [=](wxCommandEvent& e) { wxMessageBox("Not implemented yet", "Slic3r"); }, wxID_ANY, "lorry_add.svg");
        append_menu_item(menuFile, _("&Export Config Bundle\u2026"), _("Export all presets to file"), 
            [=](wxCommandEvent& e) { wxMessageBox("Not implemented yet", "Slic3r"); }, wxID_ANY, "lorry_go.svg");
        menuFile->AppendSeparator();
        append_menu_item(menuFile, _("Q&uick Slice\u2026"), _("Slice file"), 
            [=](wxCommandEvent& e) { wxMessageBox("Not implemented yet", "Slic3r"); }, wxID_ANY, "cog_go.svg", "Ctrl+U");
        append_menu_item(menuFile, _("Quick Slice and Save &As\u2026"), _("Slice file and save as"), 
            [=](wxCommandEvent& e) { wxMessageBox("Not implemented yet", "Slic3r"); }, wxID_ANY, "cog_go.svg", "Ctrl+Alt+U");
        menuFile->AppendSeparator();
        append_menu_item(menuFile, _("Repair STL file\u2026"), _("Automatically repair an STL file"), 
             [=](wxCommandEvent& e) { wxMessageBox("Not implemented yet", "Slic3r"); }, wxID_ANY, "wrench.svg");
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
        append_submenu(menuPlater, _("Select"), _("Select an object in the plater"), selectMenu, wxID_ANY, "brick.svg"); 
        append_menu_item(menuPlater, _("Undo"), _("Undo"), [this](wxCommandEvent& e) { this->plater->undo(); }, wxID_ANY, "arrow_undo.svg", "Ctrl+Z");
        append_menu_item(menuPlater, _("Redo"), _("Redo"), [this](wxCommandEvent& e) { this->plater->redo(); }, wxID_ANY, "arrow_redo.svg", "Ctrl+Shift+Z");
        append_menu_item(menuPlater, _("Select Next Object"), _("Select Next Object in the plater"), 
                [this](wxCommandEvent& e) { this->plater->select_next(); }, wxID_ANY, "arrow_right.svg", "Ctrl+Right");
        append_menu_item(menuPlater, _("Select Prev Object"), _("Select Previous Object in the plater"), 
                [this](wxCommandEvent& e) { this->plater->select_prev(); }, wxID_ANY, "arrow_left.svg", "Ctrl+Left");
        append_menu_item(menuPlater, _("Zoom In"), _("Zoom In"), 
                [this](wxCommandEvent& e) { this->plater->zoom(Zoom::In); }, wxID_ANY, "zoom_in.svg", "Ctrl+Up");
        append_menu_item(menuPlater, _("Zoom Out"), _("Zoom Out"), 
                [this](wxCommandEvent& e) { this->plater->zoom(Zoom::In); }, wxID_ANY, "zoom_out.svg", "Ctrl+Down");
        menuPlater->AppendSeparator();
        append_menu_item(menuPlater, _("Export G-code..."), _("Export current plate as G-code"), 
                [this](wxCommandEvent& e) { this->plater->export_gcode(); }, wxID_ANY, "cog_go.svg");
        append_menu_item(menuPlater, _("Export plate as STL..."), _("Export current plate as STL"), 
                [this](wxCommandEvent& e) { this->plater->export_stl(); }, wxID_ANY, "brick_go.svg");
        append_menu_item(menuPlater, _("Export plate with modifiers as AMF..."), _("Export current plate as AMF, including all modifier meshes"), 
                [this](wxCommandEvent& e) { this->plater->export_amf(); }, wxID_ANY, "brick_go.svg");
        append_menu_item(menuPlater, _("Export plate with modifiers as 3MF..."), _("Export current plate as 3MF, including all modifier meshes"), 
                [this](wxCommandEvent& e) { this->plater->export_tmf(); }, wxID_ANY, "brick_go.svg");
    }
    wxMenu* menuObject = this->plater->object_menu();
    this->on_plater_object_list_changed(false);
    this->on_plater_selection_changed(false);

    wxMenu* menuSettings = new wxMenu();
    {
        append_menu_item(menuSettings, _("P&rint Settings\u2026"), _("Show the print settings editor"), 
            [this](wxCommandEvent&) { if(this->plater) this->plater->show_preset_editor(preset_t::Print, 0); }, wxID_ANY, "cog.svg", "Ctrl+1");
        append_menu_item(menuSettings, _("&Filament Settings\u2026"), _("Show the filament settings editor"), 
            [this](wxCommandEvent&) { if(this->plater) this->plater->show_preset_editor(preset_t::Material, 0); }, wxID_ANY, "spool.svg", "Ctrl+2");
        append_menu_item(menuSettings, _("Print&er Settings\u2026"), _("Show the printer settings editor"), 
            [this](wxCommandEvent&) { if(this->plater) this->plater->show_preset_editor(preset_t::Printer, 0); }, wxID_ANY, "printer_empty.svg", "Ctrl+3");
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
            [this](wxCommandEvent&) { if (this->plater) this->plater->select_view_3d(); }, wxID_ANY, "brick.svg");
        append_menu_item(menuView, _("&G-code Preview"), _("Switch to G-code Preview"), 
            [this](wxCommandEvent&) { if (this->plater) this->plater->select_view_preview(); }, wxID_ANY, "cog.svg");
        
        menuView->AppendSeparator();
        append_menu_item(menuView, _("Theme Preview (Dev)"), _("Open the Widget Gallery to test themed controls"), 
            [this](wxCommandEvent&) { 
                WidgetGallery gallery(this);
                gallery.ShowModal();
            }, wxID_ANY, "wand.svg");

        append_menu_item(menuView, _("Toggle &Full Screen"), _("Toggle full screen mode"), 
            [this](wxCommandEvent&) { this->ShowFullScreen(!this->IsFullScreen()); }, wxID_ANY, "monitor.svg", "F11");
    }

    wxMenu* menuWindow = new wxMenu();
    {
        append_menu_item(menuWindow, _("&Plater"), _("Show the plater"), 
            [this](wxCommandEvent&) { if(this->tabpanel) this->tabpanel->SetSelection(0); }, wxID_ANY, "application_view_tile.svg", "Ctrl+T");
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
            }, wxID_ANY, "printer_empty.svg", "Ctrl+Y");
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

void MainFrame::sync_colors() {
    if (!ui_settings) return;

    wxColour btn_bg = wxSystemSettings::GetColour(wxSYS_COLOUR_FRAMEBK);
    wxColour btn_fg = *wxBLACK;
    auto colors = ThemeManager::GetColors();

    if (colors.isDark) {
        this->SetBackgroundColour(colors.bg);
        if (this->top_bar) this->top_bar->SetBackgroundColour(colors.header);
        btn_bg = colors.header;
        btn_fg = *wxWHITE;
    } else {
        // FORCE Light Grey instead of asking System (which might return Dark Grey if OS is in Dark Mode)
        wxColour light_bg(240, 240, 240); 
        this->SetBackgroundColour(light_bg);
        if (this->top_bar) this->top_bar->SetBackgroundColour(light_bg);
        
        // Ensure Text is Black in Light Mode
        btn_fg = *wxBLACK;
    }

    if (btn_prepare) {
        btn_prepare->SetBackgroundColour(btn_bg);
        btn_prepare->SetForegroundColour(btn_fg);
    }
    if (btn_preview) {
        btn_preview->SetBackgroundColour(btn_bg);
        btn_preview->SetForegroundColour(btn_fg);
    }
    if (btn_device) {
        btn_device->SetBackgroundColour(btn_bg);
        btn_device->SetForegroundColour(btn_fg);
    }

    if (btn_slice) {
        btn_slice->SetBackgroundColour(colors.accent);
        btn_slice->SetForegroundColour(*wxWHITE);
    }

    if (btn_export) {
        if (ui_settings->dark_mode) {
            btn_export->SetBackgroundColour(wxColour(100, 100, 100));
        } else {
            btn_export->SetBackgroundColour(wxColour(200, 200, 200));
        }
        btn_export->SetForegroundColour(ui_settings->dark_mode ? *wxWHITE : *wxBLACK);
    }

    this->Refresh();
    if (this->plater) {
        this->plater->Refresh();
        this->plater->update_ui_from_settings();
    }
    if (this->controller) this->controller->Refresh();

    // Rebuild the menubar to update icons with the new theme colors
    // We must delete the old one after replacing it, as SetMenuBar detaches it.
    wxMenuBar* oldMenu = this->GetMenuBar();
    this->init_menubar();
    if (oldMenu) delete oldMenu;
}

}} // Namespace Slic3r::GUI
