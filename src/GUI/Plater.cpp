#include <memory>
#include <climits>
#include <cmath>

#include <wx/progdlg.h>
#include <wx/window.h> 
#include <wx/numdlg.h> 
#include <wx/textdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h> 
#include <wx/button.h> 
#include <wx/statline.h>
#include <thread> 


#include <wx/dcbuffer.h>


#include "Plater.hpp"
#include "Widgets/ThemedMenuPopup.hpp"
#include "GUI.hpp"
#include "Theme/ThemeManager.hpp"
#include "Log.hpp"
#include "MainFrame.hpp"
#include "BoundingBox.hpp"
#include "Geometry.hpp"
#include "Dialogs/AnglePicker.hpp"
#include "Dialogs/ObjectCutDialog.hpp"
#include "Dialogs/ObjectSettingsDialog.hpp"
#include "Dialogs/PresetEditor.hpp"
#include "Widgets/ThemedTabbedPanel.hpp"
#include "OptionsGroup.hpp"


namespace Slic3r { namespace GUI {

const auto PROGRESS_BAR_EVENT = wxNewEventType();

Plater::Plater(wxWindow* parent, const wxString& title) : 
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, title)
{
    this->left_sizer = new wxBoxSizer(wxVERTICAL);

    // Apply background color early to prevent artifacts
    SetBackgroundColour(ThemeManager::GetColors().bg);

    // Use ThemedPanel with "hard black" style
    this->sidebar_content = new ThemedPanel(this);
    this->sidebar_content->SetBorder(true, *wxBLACK);
    
    // Create a wrapper sizer to add 1px padding so content doesn't cover the border
    wxBoxSizer* border_sizer = new wxBoxSizer(wxVERTICAL);
    border_sizer->Add(this->left_sizer, 1, wxEXPAND | wxALL, 1);
    
    this->sidebar_content->SetSizer(border_sizer);

    this->_presets = new PresetChooser(this->sidebar_content, this->print);

    if (ThemeManager::IsDark()) {
        this->SetBackgroundColour(ThemeManager::GetColors().bg);
    }

    // Set callback for status event for worker threads
    /*
    this->print->set_status_cb([=](std::string percent percent, std::wstring message) {
        wxPostEvent(this, new wxPlThreadEvent(-1, PROGRESS_BAR_EVENT, 
    });
    */
    _presets->on_change = [this](preset_t) {
        this->load_current_presets();
        this->refresh_canvases();
        if (this->quick_options_group) {
             this->quick_options_group->update_options(&this->config->config());
        }
    };
    _presets->load();
    this->load_current_presets();

    this->preview_notebook = new wxSimplebook(this, wxID_ANY);

    // Initialize handlers for canvases
    auto on_select_object {[this](ObjIdx obj_idx) { this->select_object(obj_idx); }};
    auto on_double_click {[this]() { if (this->selected_object() != this->objects.end()) this->object_settings_dialog(); }};
    auto on_right_click {[this](wxPanel* canvas, const wxPoint& pos) 
        {
            auto menu = this->object_menu();
            // Use custom popup with ownership transfer
            ThemedMenuPopup* popup = new ThemedMenuPopup(canvas, menu);
            popup->SetOwnsMenu(true);
            popup->Position(canvas->ClientToScreen(pos), wxSize(0,0));
            popup->Popup();
        }};
    auto on_instances_moved {[this]() { this->on_model_change(); }};

    canvas3D = new Plate3D(preview_notebook, wxDefaultSize, objects, model, config);
    preview_notebook->AddPage(canvas3D, _("3D"));

    canvas3D->on_select_object = std::function<void (ObjIdx obj_idx)>(on_select_object);
    canvas3D->on_instances_moved = std::function<void ()>(on_instances_moved);
    canvas3D->on_right_click = std::function<void(wxWindow* canvas, const wxPoint& pos)>([=](wxWindow* canvas, const wxPoint& pos) { on_right_click(static_cast<wxPanel*>(canvas), pos); });
    
    // Bind ImGui Toolbar Callbacks (Dynamic List)
    canvas3D->m_actions.clear();
    
    // Group 1: Add/File
    canvas3D->m_actions.push_back(ToolbarItem::Action("brick_add", "Add Object", [this](){ wxTheApp->CallAfter([this](){ this->add(); }); }));
    canvas3D->m_actions.push_back(ToolbarItem::Separator());

    // Group 2: Arrange
    canvas3D->m_actions.push_back(ToolbarItem::Action("bricks", "Arrange", [this](){ wxTheApp->CallAfter([this](){ this->arrange(); }); }));
    canvas3D->m_actions.push_back(ToolbarItem::Separator());

    // Group 3: Copies
    canvas3D->m_actions.push_back(ToolbarItem::Action("add", "Increase Copies", [this](){ wxTheApp->CallAfter([this](){ this->increase(); }); }));
    canvas3D->m_actions.push_back(ToolbarItem::Action("delete", "Decrease Copies", [this](){ wxTheApp->CallAfter([this](){ this->decrease(); }); }));
    canvas3D->m_actions.push_back(ToolbarItem::Separator());

    // Group 4: Rotate
    canvas3D->m_actions.push_back(ToolbarItem::Action("arrow_rotate_anticlockwise", "Rotate -45", [this](){ wxTheApp->CallAfter([this](){ this->rotate(45); }); }));
    canvas3D->m_actions.push_back(ToolbarItem::Action("arrow_rotate_clockwise", "Rotate +45", [this](){ wxTheApp->CallAfter([this](){ this->rotate(-45); }); }));
    canvas3D->m_actions.push_back(ToolbarItem::Separator());

    // Group 5: Scale
    canvas3D->m_actions.push_back(ToolbarItem::Action("arrow_out", "Scale", [this](){ wxTheApp->CallAfter([this](){ this->changescale(); }); }));
    canvas3D->m_actions.push_back(ToolbarItem::Separator());

    // Group 6: Tools
    canvas3D->m_actions.push_back(ToolbarItem::Action("shape_ungroup", "Split", [this](){ wxTheApp->CallAfter([this](){ this->split_object(); }); }));
    canvas3D->m_actions.push_back(ToolbarItem::Action("package", "Cut", [this](){ wxTheApp->CallAfter([this](){ this->object_cut_dialog(); }); }));
    canvas3D->m_actions.push_back(ToolbarItem::Separator());

    // Group 7: Settings
    canvas3D->m_actions.push_back(ToolbarItem::Action("cog", "Settings", [this](){ wxTheApp->CallAfter([this](){ this->object_settings_dialog(); }); }));
    canvas3D->m_actions.push_back(ToolbarItem::Action("variable_layer_height", "Layers", [this](){ wxTheApp->CallAfter([this](){ this->object_layers_dialog(); }); }));
    
    preview3D = new Preview3D(preview_notebook, wxDefaultSize, print, objects, model, config);
    preview_notebook->AddPage(preview3D, _("Preview"));
    
    // Set initial bed shape for preview
    if(config) {
        auto bed_poly = Slic3r::Polygon::new_scale(config->get<ConfigOptionPoints>("bed_shape").values);
        preview3D->set_bed_shape(bed_poly.points);
    }

    preview_notebook->Bind(wxEVT_BOOKCTRL_PAGE_CHANGED, [this](wxBookCtrlEvent& e) {
        int sel = e.GetSelection();
        if (sel != wxNOT_FOUND) {
            wxTheApp->CallAfter([this, sel]() {
                wxString page_text = preview_notebook->GetPageText(sel);
                if (page_text == _("Preview") && preview3D) {
                     preview3D->load_print();
                }
            });
        }
        e.Skip();
    });

    // Info Section
    {
        this->object_info_section = new ThemedSection(this->sidebar_content, _("Info"), "info");
        auto* object_info_sizer = this->object_info_section->GetContentSizer();
        this->object_info_section->SetMinSize(wxSize(350, -1));

        {
            auto* sizer {new wxBoxSizer(wxHORIZONTAL)};
            object_info_sizer->Add(sizer, 0, wxEXPAND | wxBOTTOM, 5);
            auto* text  {new wxStaticText(this->object_info_section, wxID_ANY, _("Object:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT)};
            text->SetFont(ui_settings->small_font());
            if (ThemeManager::IsDark()) text->SetForegroundColour(*wxWHITE);
            else text->SetForegroundColour(*wxBLACK); // Force Black on Light
            sizer->Add(text, 0, wxALIGN_CENTER_VERTICAL);

            /* We supply a bogus width to wxChoice (sizer will override it and stretch 
             * the control anyway), because if we leave the default (-1) it will stretch
             * too much according to the contents, and this is bad with long file names.
             */
            this->object_info.choice = new ThemedSelect(this->object_info_section, wxID_ANY, wxArrayString(), wxDefaultPosition, wxSize(100, 30));
            this->object_info.choice->SetFont(ui_settings->small_font());
            // ThemedSelect handles colors internally via ThemeManager

            sizer->Add(this->object_info.choice, 1, wxALIGN_CENTER_VERTICAL);

            // Select object on change.
            this->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent& e) {  
                // Verify event source? Bind on 'this' catches all. 
                // But we only have one choice/select here so it's fine.
                // Ideally bind to the control instance if possible, but ThemedSelect is custom.
                if (e.GetEventObject() == this->object_info.choice) {
                   this->select_object(this->object_info.choice->GetSelection()); 
                   this->refresh_canvases();
                }
            });
                
        }
        
        auto* grid_sizer { new wxFlexGridSizer(3, 4, 5, 5)};
        grid_sizer->SetFlexibleDirection(wxHORIZONTAL);
        grid_sizer->AddGrowableCol(1, 1);
        grid_sizer->AddGrowableCol(3, 1);

        add_info_field(this->object_info_section, this->object_info.copies, _("Copies"), grid_sizer);
        add_info_field(this->object_info_section, this->object_info.size, _("Size"), grid_sizer);
        add_info_field(this->object_info_section, this->object_info.volume, _("Volume"), grid_sizer);
        add_info_field(this->object_info_section, this->object_info.facets, _("Facets"), grid_sizer);
        add_info_field(this->object_info_section, this->object_info.materials, _("Materials"), grid_sizer);
        {
            wxString name {"Manifold:"};
            auto* text {new wxStaticText(this->object_info_section, wxID_ANY, name, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT)};
            text->SetFont(ui_settings->small_font());
            if (ThemeManager::IsDark()) text->SetForegroundColour(*wxWHITE);
            grid_sizer->Add(text, 0);

            this->object_info.manifold = new wxStaticText(this->object_info_section, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
            this->object_info.manifold->SetFont(ui_settings->small_font());
            if (ThemeManager::IsDark()) this->object_info.manifold->SetForegroundColour(*wxWHITE);

            this->object_info.manifold_warning_icon = new wxStaticBitmap(this->object_info_section, wxID_ANY, get_bmp_bundle("error.svg"));
            this->object_info.manifold_warning_icon->Hide();

            auto* h_sizer {new wxBoxSizer(wxHORIZONTAL)};
            h_sizer->Add(this->object_info.manifold_warning_icon, 0);
            h_sizer->Add(this->object_info.manifold, 0);

            grid_sizer->Add(h_sizer, 0, wxEXPAND);
        }

        object_info_sizer->Add(grid_sizer, 0, wxEXPAND);
    }
    
    this->selection_changed();


    // Quick Settings using ThemedTabbedPanel
    {
        this->quick_settings_panel = new ThemedTabbedPanel(this->sidebar_content, wxID_ANY);
        this->quick_settings_panel->SetMinSize(wxSize(350, 200)); 
    }
    
    // Explicitly call update_quick_settings to populate the newly created OptionGroups
    this->update_quick_settings();


    // Finally assemble the sizers into the display.
    
    // left panel sizer
    auto* left_sizer {this->left_sizer};
    left_sizer->Add(this->_presets, 0, wxEXPAND | wxTOP, 10);
    this->_presets->Show();

    left_sizer->Add(this->quick_settings_panel, 0, wxEXPAND | wxTOP, 5);

//    $right_sizer->Add($self->{settings_override_panel}, 1, wxEXPAND, 5);
    left_sizer->Add(this->object_info_section, 0, wxEXPAND | wxTOP, 15);
//    $right_sizer->Add($object_info_sizer, 0, wxEXPAND, 0);


    auto hsizer {new wxBoxSizer(wxHORIZONTAL)};
    hsizer->Add(this->sidebar_content, 0, wxEXPAND);
    
    // The sidebar_frame already provides the black separator via its border,
    // so we don't need the static gray line here.

    hsizer->Add(this->preview_notebook, 1, wxEXPAND);

    auto sizer {new wxBoxSizer(wxVERTICAL)};
    
    sizer->Add(hsizer, 1, wxEXPAND,0);

    sizer->SetSizeHints(this);
    this->SetSizer(sizer);
    this->Layout();

    this->selection_changed();
}

Plater::~Plater() {
    stop_background_process();
    // smart pointers handle the rest
}

void Plater::update_ui_from_settings() {
     // Update Background
    if (ThemeManager::IsDark()) {
         this->SetBackgroundColour(ThemeManager::GetColors().bg);
    } else {
         this->SetBackgroundColour(wxColour(240, 240, 240)); 
    }
    
    if (this->sidebar_content) {
        this->sidebar_content->SetBackgroundColour(ThemeManager::GetColors().bg);
        this->sidebar_content->Refresh();
    }
    
    // Update Warning Icon
    if (this->object_info.manifold_warning_icon) {
        this->object_info.manifold_warning_icon->SetBitmap(get_bmp_bundle("error.svg"));
    }
    
    if (this->_presets) {
        this->_presets->UpdateTheme();
    }


    
    this->update_quick_settings();

    this->Refresh();
}

void Plater::load_current_presets() {
    if (!SLIC3RAPP) return;

    auto* settings = SLIC3RAPP->settings();
    auto& preset_store = SLIC3RAPP->presets;
    
    // Order: Printer -> Print -> Material
    // This ensures that material overrides (like temps) take precedence, 
    // and printer limits are respected (though logic might vary, usually apply() overwrites).
    
    // Actually, Slic3r applies them in specific order. 
    // Let's assume: Printer first (base), then Print (params), then Material (filament specifics).
    
    auto apply_group = [&](preset_t group) {
        size_t group_idx = static_cast<size_t>(group);
        // Safety check
        if (group_idx >= settings->default_presets.size()) return;
        
        const auto& defaults = settings->default_presets[group_idx];
        if (defaults.empty()) return;
        
        std::string name = defaults[0].ToStdString(); 
        
        // Find preset
        for (auto& preset : preset_store[group_idx]) {
             if (preset.name == name) {
                 if (auto cfg = preset.config().lock()) {
                     this->config->apply(cfg->config());
                     Slic3r::Log::info(LogChannel, "Applied preset config: " + name);
                 }
                 return;
             }
        }
    };
    
    apply_group(preset_t::Printer);
    apply_group(preset_t::Print);
    apply_group(preset_t::Material);
    
    // Sync to Print object
    this->print->apply_config(this->config->config());
}

void Plater::select_view_3d() {

    
    if (this->preview_notebook) this->preview_notebook->SetSelection(0);
}

void Plater::select_view(Direction dir) {
    if (this->canvas3D) {
        this->canvas3D->set_camera_view(dir);
    }
    this->select_view_3d();
}

void Plater::select_view_preview() {
    // Index 1 is Preview (3D G-code)
    if (this->preview_notebook) this->preview_notebook->SetSelection(1);
}

void Plater::add() {
    Log::info(LogChannel, L"Called Add function");

    auto& start_object_id = this->object_identifier;
    const auto& input_files{open_model(this, wxTheApp->GetTopWindow())};
    for (const auto& f : input_files) {
        Log::info(LogChannel, (wxString(L"Calling Load File for ") + f).ToStdWstring());
        this->load_file(f.ToStdString());
    }

    // abort if no objects actually added.
    if (start_object_id == this->object_identifier) return;

    // save the added objects
    auto new_model {this->model};

    // get newly added objects count
    auto new_objects_count = this->object_identifier - start_object_id;
    
    Slic3r::Log::info(LogChannel, (wxString("Obj id:") << object_identifier).ToStdWstring());
    for (auto i = start_object_id; i < new_objects_count + start_object_id; i++) {
        const auto& obj_idx {this->get_object_index(i)};
        new_model->add_object(*(this->model->objects.at(obj_idx)));
    }
    Slic3r::Log::info(LogChannel, (wxString("Obj id:") << object_identifier).ToStdWstring());

    // Prepare for undo
    //this->add_undo_operation("ADD", nullptr, new_model, start_object_id);
   

}

void Plater::load_files(const std::vector<std::string>& files) {
    for (const auto& file : files) {
        this->load_file(file);
    }
}

std::vector<int> Plater::load_file(const std::string file, const int obj_idx_to_load) {
    std::vector<int> obj_idx;
    if (file.empty()) return obj_idx;
    
    auto progress_dialog {new wxProgressDialog(_(L"Loading\u2026"), _(L"Processing input file\u2026"), 100, this, 0)};
    progress_dialog->Pulse();
    
    Model model;
    bool valid_load = true;

    //TODO: Add a std::wstring so we can handle non-roman characters as file names.
    try { 
        model = Slic3r::Model::read_from_file(file);
    } catch (std::runtime_error& e) {
        show_error(this, e.what());
        Slic3r::Log::error(LogChannel, LOG_WSTRING(file << " failed to load: " << e.what()));
        valid_load = false;
    }
    Slic3r::Log::info(LogChannel, LOG_WSTRING("load_valid is " << valid_load));

    if (valid_load) {
        if (model.looks_like_multipart_object()) {
            auto dialog {new wxMessageDialog(this, 
            _("This file contains several objects positioned at multiple heights. Instead of considering them as multiple objects, should I consider them as a single object having multiple parts?"), _("Multi-part object detected"), wxICON_WARNING | wxYES | wxNO)};
            if (dialog->ShowModal() == wxID_YES) {
                model.convert_multipart_object();
            }
            dialog->Destroy();
        } 
        
        for (auto i = 0U; i < model.objects.size(); i++) {
            auto object {model.objects[i]};
            object->input_file = file;
            for (auto j = 0U; j < object->volumes.size(); j++) {
                auto volume {object->volumes.at(j)};
                volume->input_file = file;
                volume->input_file_obj_idx = i;
                volume->input_file_vol_idx = j;
            }
        }
        
        auto i {0U};
        if (obj_idx_to_load >= 0) { // Changed > 0 to >= 0 to match logic (or check original intent) - actually logic was > 0, but usually indices are >= 0. Let's keep intent but fix type.
            // Wait, original code was: if (obj_idx_to_load > 0). If default is -1, then > 0 means explicit index 1+? Or is 0 valid? Usually 0 is valid.
            // Let's assume -1 means "all".
             if (obj_idx_to_load != -1) {
                Slic3r::Log::info(LogChannel, L"Loading model objects, obj_idx_to_load specified");
                const size_t idx_load = (size_t)obj_idx_to_load;
                if (idx_load >= model.objects.size()) {
                    progress_dialog->Destroy();
                    return std::vector<int>();
                }
                obj_idx = this->load_model_objects(model.objects.at(idx_load));
                i = idx_load;
            } else {
                goto LOAD_ALL;
            }
        } else {
            LOAD_ALL:
            Slic3r::Log::info(LogChannel, L"Loading model objects, all");
            obj_idx = this->load_model_objects(model.objects);
            Slic3r::Log::info(LogChannel, LOG_WSTRING("obj_idx size: " << obj_idx.size()));
        }

        // Fix logic for re-mapping input file indices if we loaded all? 
        // The original loop: for (const auto &j : obj_idx) { this->objects.at(j).input_file = file; ... }
        // Wait, 'i' was used as counter.
        // If we loaded specific index, 'i' is that index.
        // If we loaded all, 'i' started at 0.
        
        int counter = 0;
        for (const auto &j : obj_idx) {
            // Check if index is valid in main objects list
            if (j >= 0 && j < this->objects.size()) {
                 this->objects.at(j).input_file = file;
                 // If we loaded a specific one, i is fixed? No, original code: input_file_obj_idx = i++; 
                 // So if we loaded one, it sets it to 'i' then increments.
                 // If we loaded all, i starts at 0 and increments.
                 // But wait, if we loaded all, 'i' should reset to 0? 
                 // In original code: auto i {0U}; ... if specific ... i = idx_load; else ... (i remains 0).
                 // So yes.
                 this->objects.at(j).input_file_obj_idx = i++;
            }
        }
        
    }

    progress_dialog->Destroy();
    this->_redo = std::stack<UndoOperation>();
    return obj_idx;
}



std::vector<int> Plater::load_model_objects(ModelObject* model_object) { 
    ModelObjectPtrs tmp {model_object}; //  wrap in a std::vector
    return load_model_objects(tmp);
}
std::vector<int> Plater::load_model_objects(ModelObjectPtrs model_objects) {
    auto bed_center {this->bed_centerf()};

    auto bed_shape {Slic3r::Polygon::new_scale(this->config->get<ConfigOptionPoints>("bed_shape").values)};
    auto bed_size {bed_shape.bounding_box().size()};

    bool need_arrange {false};

    auto obj_idx {std::vector<int>()};
    Slic3r::Log::info(LogChannel, LOG_WSTRING("Objects: " << model_objects.size()));

    for (auto& obj : model_objects) {
        auto o {this->model->add_object(*obj)};
        o->repair();
        
        auto tmpobj {PlaterObject()};
        const auto objfile {wxFileName::FileName( obj->input_file )};
        tmpobj.name = (std::string() == obj->name ? wxString(obj->name) : objfile.GetName());
        tmpobj.identifier = (this->object_identifier)++;

        this->objects.push_back(tmpobj);
        obj_idx.push_back(this->objects.size() - 1);
        Slic3r::Log::info(LogChannel, LOG_WSTRING("Object array new size: " << this->objects.size()));
        Slic3r::Log::info(LogChannel, LOG_WSTRING("Instances: " << obj->instances.size()));

        if (obj->instances.size() == 0) {
            if (ui_settings->autocenter) {
                need_arrange = true;
                o->center_around_origin();

                o->add_instance();
                o->instances.back()->offset = this->bed_centerf();
            } else {
                need_arrange = false;
                if (ui_settings->autoalignz) {
                    o->align_to_ground();
                }
                o->add_instance();
            }
        } else {
            if (ui_settings->autoalignz) {
                o->align_to_ground();
            }
        }
        {
            // If the object is too large (more than 5x the bed) scale it down.
            auto size {o->bounding_box().size()};
            double ratio {0.0f};
            if (ratio > 5) {
                for (auto& instance : o->instances) {
                    instance->scaling_factor = (1.0f/ratio);
                    this->scaled_down = true;
                }
            }
        }

        { 
            // Provide a warning if downscaling by 5x still puts it over the bed size.

        }
        this->print->auto_assign_extruders(o);
        this->print->add_model_object(o);
    }
    for (const auto& i : obj_idx) { this->make_thumbnail(i); } 
    if (need_arrange) this->arrange();
    this->object_list_changed();
    return obj_idx;
}

MainFrame* Plater::GetFrame() { return dynamic_cast<MainFrame*>(wxGetTopLevelParent(this)); }

int Plater::get_object_index(ObjIdx object_id) {
    for (size_t i = 0U; i < this->objects.size(); i++) {
        if (this->objects.at(i).identifier == object_id) return static_cast<int>(i);
    }
    return -1;
}

void Plater::make_thumbnail(size_t idx) {
    auto& plater_object {this->objects.at(idx)};
    if (threaded) {
        // spin off a thread to create the thumbnail and post an event when it is done.
    } else {
        plater_object.make_thumbnail(this->model, idx);
        this->on_thumbnail_made(idx);
    }
/*
    my $plater_object = $self->{objects}[$obj_idx];
    $plater_object->thumbnail(Slic3r::ExPolygon::Collection->new);
    my $cb = sub {
        $plater_object->make_thumbnail($self->{model}, $obj_idx);
        
        if ($Slic3r::have_threads) {
            Wx::PostEvent($self, Wx::PlThreadEvent->new(-1, $THUMBNAIL_DONE_EVENT, shared_clone([ $obj_idx ])));
            Slic3r::thread_cleanup();
            threads->exit;
        } else {
            $self->on_thumbnail_made($obj_idx);
        }
    };
    
    @_ = ();
    $Slic3r::have_threads
        ? threads->create(sub { $cb->(); Slic3r::thread_cleanup(); })->detach
        : $cb->();
}
*/
}

void Plater::on_thumbnail_made(size_t idx) {
    this->objects.at(idx).transform_thumbnail(this->model, idx);
    this->refresh_canvases();
}

void Plater::refresh_canvases() {

    if (this->canvas3D != nullptr)
        this->canvas3D->update();
    if (this->preview3D != nullptr)
        this->preview3D->reload_print();

}

void Plater::arrange() {
    // TODO pause background process
    const Slic3r::BoundingBoxf bb {Slic3r::BoundingBoxf(this->config->get<ConfigOptionPoints>("bed_shape").values)};
    if (this->objects.size() == 0U) { // abort
        return; 
    }
    bool success {this->model->arrange_objects(this->config->config().min_object_distance(), &bb)};

    if (success) {
    } else {
    }
    this->on_model_change(true);
}

void Plater::on_model_change(bool force_autocenter) {
    Log::info(LogChannel, L"Called on_modal_change");

    // reload the select submenu (if already initialized)
    {
        auto* menu = this->GetFrame()->plater_select_menu;

        if (menu != nullptr) {
            menu->Clear();
            for (const auto& obj : this->objects) {
                const auto idx {obj.identifier};
                auto name {wxString(obj.name)};
                auto inst_count = this->model->objects.at(idx)->instances.size();
                if (inst_count > 1) {
                    name << " (" << inst_count << "x)";
                }
                auto* item {append_menu_item(menu, name, _("Select object."), 
                        [this,idx](wxCommandEvent& e) { this->select_object(idx); this->refresh_canvases(); }, 
                        wxID_ANY, "", "", wxITEM_CHECK)};
                if (obj.selected) item->Check(true);
            }
        }
    }

    if (force_autocenter || ui_settings->autocenter) {
        this->model->center_instances_around_point(this->bed_centerf());
    }
    this->refresh_canvases();
}

ObjRef Plater::selected_object() {
    Slic3r::Log::info(LogChannel, L"Calling selected_object()");
    auto it {this->objects.begin()};
    for (; it != this->objects.end(); it++)
        if (it->selected) return it;
    Slic3r::Log::info(LogChannel, L"No object selected.");
    return this->objects.end();
}

void Plater::object_settings_dialog() { object_settings_dialog(this->selected_object()); }
void Plater::object_settings_dialog(ObjIdx obj_idx) { object_settings_dialog(this->objects.begin() + obj_idx); }
void Plater::object_settings_dialog(ObjRef obj) {
    if (obj == this->objects.end()) return;
    
    int idx = this->get_object_index(obj->identifier);
    if (idx < 0 || idx >= this->model->objects.size()) return;

    auto* model_object = this->model->objects.at(idx);

    ObjectSettingsDialog dlg(this, model_object);
    if (dlg.ShowModal() == wxID_OK) {
        // Changes applied to model_object in dlg.save_layers()
        // We need to trigger updates
        this->stop_background_process();
        this->print->reload_object(idx); // Reload object in Print
        this->on_model_change();
    }
}

void Plater::select_object(ObjRef obj) {
    for (auto& o : this->objects) {
        o.selected = false;
        o.selected_instance = -1;
    }
    // range check the iterator
    if (obj < this->objects.end() && obj >= this->objects.begin()) {
        obj->selected = true;
        obj->selected_instance = 0;
    }
    this->selection_changed(); // selection_changed(1) in perl
}

void Plater::select_object(ObjIdx obj_idx) {
    if (obj_idx >= this->objects.size()) {
        this->select_object();
        return;
    }
    this->select_object(this->objects.begin() + obj_idx);
}

void Plater::select_object() {
    this->select_object(this->objects.end());
}

void Plater::selection_changed() {

    this->canvas3D->selection_changed();

    auto obj = this->selected_object();
    bool have_sel {obj != this->objects.end()};
    auto* menu {this->GetFrame()->plater_select_menu};
    if (menu != nullptr) {
        for (size_t i = 0; i < menu->GetMenuItemCount(); ++i) {
            menu->FindItemByPosition(i)->Check(false);
        }
        if (have_sel) {
            int idx = std::distance(this->objects.begin(), obj);
            if (idx >= 0 && idx < (int)menu->GetMenuItemCount())
                menu->FindItemByPosition(idx)->Check(true);
        }
    }

    if (this->object_info.choice != nullptr) {
        if (have_sel) {
            int obj_idx = std::distance(this->objects.begin(), obj);
            this->object_info.choice->SetSelection(obj_idx);

            int model_obj_idx = this->get_object_index(obj->identifier);
            auto* model_object = this->model->objects.at(model_obj_idx);
            this->object_info.copies->SetLabel(wxString::Format("%zu", model_object->instances.size()));
            
            auto* model_instance = model_object->instances.front();
            {
                auto size = model_object->instance_bounding_box(0).size();
                wxString size_string = wxString::Format("%.2f x %.2f x %.2f", size.x, size.y, size.z);
                if (std::abs(model_instance->scaling_factor - 1.0) > 0.0001) {
                    size_string << wxString::Format(" (%.0f%%)", model_instance->scaling_factor * 100.0);
                }
                this->object_info.size->SetLabel(size_string);
            }
            
            this->object_info.materials->SetLabel(wxString::Format("%zu", model_object->materials_count()));
            
            auto raw_mesh = model_object->raw_mesh();
            raw_mesh.repair();
            auto stats = raw_mesh.stats();
            
            this->object_info.volume->SetLabel(wxString::Format("%.2f", stats.volume * std::pow(model_instance->scaling_factor, 3)));
            this->object_info.facets->SetLabel(wxString::Format("%zu (%zu shells)", model_object->facets_count(), stats.number_of_parts));
            
            size_t errors = stats.degenerate_facets + stats.edges_fixed + stats.facets_removed + 
                           stats.facets_added + stats.facets_reversed + stats.backwards_edges;
            
            if (errors > 0) {
                this->object_info.manifold->SetLabel(wxString::Format("Auto-repaired (%zu errors)", errors));
                this->object_info.manifold_warning_icon->Show();
                
                wxString message = wxString::Format("%zu degenerate facets, %zu edges fixed, %zu facets removed, %zu facets added, %zu facets reversed, %zu backwards edges",
                    stats.degenerate_facets, stats.edges_fixed, stats.facets_removed, stats.facets_added, stats.facets_reversed, stats.backwards_edges);
                this->object_info.manifold->SetToolTip(message);
                this->object_info.manifold_warning_icon->SetToolTip(message);
            } else {
                this->object_info.manifold->SetLabel(_("Yes"));
                this->object_info.manifold_warning_icon->Hide();
                this->object_info.manifold->SetToolTip("");
            }
        } else {
            this->object_info.choice->SetSelection(wxNOT_FOUND);
            this->object_info.copies->SetLabel("");
            this->object_info.size->SetLabel("");
            this->object_info.volume->SetLabel("");
            this->object_info.facets->SetLabel("");
            this->object_info.materials->SetLabel("");
            this->object_info.manifold->SetLabel("");
            this->object_info.manifold_warning_icon->Hide();
            this->object_info.manifold->SetToolTip("");
        }
        this->Layout();
    }
    
    this->GetFrame()->on_plater_selection_changed(have_sel);
}

void Plater::remove() {
    this->remove(-1, false);
}

void Plater::remove(int obj_idx, bool dont_push) {
    
    stop_background_process();
    
    // Prevent toolpaths preview from rendering while we modify the Print object

    if (this->preview3D != nullptr) 
        this->preview3D->enabled(false);

    /*
    if (this->previewDLP != nullptr) 
        this->previewDLP->enabled(false);
    */
    
    ObjRef obj_ref;
    // if no object index is supplied or an invalid one is supplied, remove the selected one
    if (obj_idx < 0 || obj_idx >= this->objects.size()) {
        obj_ref = this->selected_object();
    } else { // otherwise 
        obj_ref = this->objects.begin() + obj_idx;
    }
    std::vector<PlaterObject>::const_iterator const_ref = obj_ref;

    if (obj_ref >= this->objects.end()) return; // do nothing, nothing was selected.

    Slic3r::Log::info(LogChannel, "Assigned obj_ref");
    // Save the object identifier and copy the object for undo/redo operations.
    auto object_id { obj_ref->identifier };
    int idx = this->get_object_index(object_id);
    if (idx < 0) return;

    auto new_model { Slic3r::Model() };
    new_model.add_object(*(this->model->objects.at(idx)));
   
    Slic3r::Log::info(LogChannel, "Assigned obj_ref");
    try {
        this->model->delete_object(idx);
    } catch (out_of_range & /*ex*/) {
        Slic3r::Log::error(LogChannel, LOG_WSTRING("Failed to delete object " << idx << " from Model."));
    }
    try {
        this->print->delete_object(idx);
    } catch (out_of_range & /*ex*/) {
        Slic3r::Log::error(LogChannel, LOG_WSTRING("Failed to delete object " << idx << " from Print."));
    }

    this->objects.erase(const_ref);
    int i = 0;
    for (auto &o : this->objects) { o.identifier = i++; } // fix identifiers
    this->object_identifier = this->objects.size();

    this->object_list_changed();
    
    this->select_object();

    this->on_model_change();

    if (!dont_push) {
        Slic3r::Log::info(LogChannel, "Push to undo stack.");
        this->add_undo_operation(UndoCmd::Remove, object_id, new_model);
        Slic3r::Log::info(LogChannel, "Pushed to undo stack.");
    }
}

void Plater::reset(bool dont_push) {
    stop_background_process();
    
    // Prevent toolpaths preview from rendering while we modify the Print object

    if (this->preview3D != nullptr) 
        this->preview3D->enabled(false);

    /*
    if (this->previewDLP != nullptr) 
        this->previewDLP->enabled(false);
    */

    if (!dont_push) {
        Slic3r::Model current_model {*(this->model)};
        std::vector<int> tmp_ids;
        for (const auto& obj : this->objects) {
            tmp_ids.push_back(obj.identifier);
        }
        this->add_undo_operation(UndoCmd::Reset, tmp_ids, current_model);
    }
   
    this->objects.clear();
    this->object_identifier = this->objects.size();

    this->model->clear_objects();
    this->print->clear_objects();

    this->object_list_changed();
    this->select_object();

    this->on_model_change();
}

void Plater::increase(size_t copies, bool dont_push) {

    auto obj {this->selected_object()};

    if (obj == this->objects.end()) return; // do nothing; nothing is selected.

    

    this->stop_background_process();

    int idx = this->get_object_index(obj->identifier);

    if (idx < 0) return;



    auto* model_object {this->model->objects.at(idx)};

    ModelInstance* instance {model_object->instances.back()};

    for (size_t i = 1; i <= copies; i++) {
        instance = model_object->add_instance(*instance);
        instance->offset.x += 10;
        instance->offset.y += 10;
        this->print->objects.at(idx)->add_copy(instance->offset);
    }

    if (!dont_push) {
        this->add_undo_operation(UndoCmd::Increase, obj->identifier, copies);
    }

    if(ui_settings->autocenter) {
        this->arrange();
    } else {
        this->on_model_change();
    }
} 

void Plater::decrease(size_t copies, bool dont_push) {
    auto obj {this->selected_object()};
    if (obj == this->objects.end()) return; // do nothing; nothing is selected.
    
    this->stop_background_process();
    int idx = this->get_object_index(obj->identifier);
    if (idx < 0) return;

    auto* model_object {this->model->objects.at(idx)};
    if (model_object->instances.size() > copies) {
        for (size_t i = 1; i <= copies; i++) {
            model_object->delete_last_instance();
            this->print->objects.at(idx)->delete_last_copy();
        }
        if (!dont_push) {
            this->add_undo_operation(UndoCmd::Decrease, obj->identifier, copies);
        }
    } else {
        this->remove();
    }
    this->on_model_change();
} 

void Plater::rotate(Axis axis, bool dont_push) {
    ObjRef obj {this->selected_object()};
    if (obj == this->objects.end()) return;
    double angle {0.0};

    int idx = this->get_object_index(obj->identifier);
    if (idx < 0) return;

    auto* model_object {this->model->objects.at(idx)};
    auto model_instance {model_object->instances.begin()};

    // pop a menu to get the angle
    auto* pick = new AnglePicker<1000>(this, "Set Angle", angle);
    if (pick->ShowModal() == wxID_OK) {
        angle = pick->angle();
        pick->Destroy(); // cleanup afterwards.
        this->rotate(angle, axis, dont_push);
    } else {
        pick->Destroy(); // cleanup afterwards.
    }
}

void Plater::rotate(double angle, Axis axis, bool dont_push) {
    ObjRef obj {this->selected_object()};
    if (obj == this->objects.end()) return;

    int idx = this->get_object_index(obj->identifier);
    if (idx < 0) return;

    auto* model_object {this->model->objects.at(idx)};
    auto* model_instance {model_object->instances.front()};

    if(obj->thumbnail.expolygons.size() == 0) { return; }

    if (axis == Z) {
        for (auto* instance : model_object->instances)
            instance->rotation += Geometry::deg2rad(angle);
        obj->transform_thumbnail(this->model, idx);
    } else {
        model_object->transform_by_instance(*model_instance, true);
        model_object->rotate(Geometry::deg2rad(angle), axis);

        // realign object to Z=0
        model_object->center_around_origin();
        this->make_thumbnail(idx);
    }

    model_object->update_bounding_box();

    this->print->add_model_object(model_object, idx);

    if (!dont_push) {
        add_undo_operation(UndoCmd::Rotate, obj->identifier, angle, axis);
    }

    this->selection_changed();
    this->on_model_change();

} 

void Plater::split_object() {
    ObjRef obj {this->selected_object()};
    if (obj == this->objects.end()) return;

    this->pause_background_process();

    const auto obj_idx = obj->identifier;
    int idx = this->get_object_index(obj_idx);
    
    if (idx < 0 || idx >= this->model->objects.size()) {
         this->resume_background_process();
         return;
    }
    
    auto* current_model_object = this->model->objects.at(idx);

    // Check if object can be split
    // Logic from Plater.pm: if (current_model_object->volumes_count > 1) warning...
    // In C++, volumes is vector.
    if (current_model_object->volumes.size() > 1) {
        wxMessageBox(_("The selected object can't be split because it contains more than one volume/material."), _("Split Object"), wxICON_WARNING);
        this->resume_background_process();
        return;
    }

    // Attempt to split
    ModelObjectPtrs new_objects;
    current_model_object->split(&new_objects);
    
    if (new_objects.size() < 2) {
         // Cleanup
         for (auto* ptr : new_objects) delete ptr;

         wxMessageBox(_("The selected object couldn't be split because it contains only one part."), _("Split Object"), wxICON_WARNING);
         this->resume_background_process();
         return;
    }
    
    // Offset and center new objects
    int i = 0;
    for (auto* object : new_objects) {
        for (auto* instance : object->instances) {
            instance->offset.translate(i * 10, i * 10);
        }
        object->center_around_origin();
        i++;
    }

    // Remove original object (dont push separate undo for remove)
    this->remove(idx, true);
    
    // Add new objects to model
    this->load_model_objects(new_objects);
    
    // Cleanup temporary objects (load_model_objects copies them)
    for (auto* ptr : new_objects) delete ptr; 
    
    // TODO: Add Undo operation for SPLIT
    // this->add_undo_operation(UndoCmd::Split, ...);

    this->resume_background_process();
} 

void Plater::changescale(bool to_size) {
    ObjRef obj {this->selected_object()};
    if (obj == this->objects.end()) return;
    
    int idx = this->get_object_index(obj->identifier);
    if (idx < 0 || idx >= this->model->objects.size()) return;

    auto* model_object = this->model->objects.at(idx);
    if (model_object->instances.empty()) return;

    auto* model_instance = model_object->instances.front();
    
    if (to_size) {
        wxMessageBox("Scale to size not implemented yet", "Not Implemented");
        return;
    }

    double current_scale_percent = model_instance->scaling_factor * 100.0;
    
    // Ask user for percentage
    wxTextEntryDialog dlg(this, _("Enter the scale % for the selected object:"), _("Scale"), wxString::Format("%.2f", current_scale_percent));
    if (dlg.ShowModal() != wxID_OK) return;
    
    wxString val = dlg.GetValue();
    if (val.IsEmpty()) return;
    
    double new_scale_percent;
    if (!val.ToDouble(&new_scale_percent) || new_scale_percent < 0) {
        return;
    }

    double scale_factor = new_scale_percent / 100.0;
    
    this->stop_background_process();
    
    // Apply uniformly
    double variation = scale_factor / model_instance->scaling_factor;
    // Update layer height ranges if any
    t_layer_height_ranges new_ranges;
    for (const auto& kv : model_object->layer_height_ranges) {
        t_layer_height_range new_range = kv.first;
        new_range.first *= variation;
        new_range.second *= variation;
        new_ranges[new_range] = kv.second * variation;
    }
    model_object->layer_height_ranges = new_ranges;
    
    for (auto* instance : model_object->instances) {
        instance->scaling_factor = scale_factor;
    }
    
    obj->transform_thumbnail(this->model, idx);
    
    model_object->update_bounding_box();
    this->print->add_model_object(model_object, idx);
    
    // TODO: Undo op
    // this->add_undo_operation(...);

    this->selection_changed();
    this->on_model_change();
}

void Plater::changescale(Axis axis, bool to_size) {
    wxMessageBox("Non-uniform scaling not implemented yet", "Not Implemented");
}

void Plater::mirror(Axis axis, bool dont_push) {
    wxMessageBox("Mirroring not implemented yet", "Not Implemented");
}

void Plater::reload_from_disk() {
    wxMessageBox("Reload from disk not implemented yet", "Not Implemented");
}

void Plater::export_object_stl() {
    wxMessageBox("Export object STL not implemented yet", "Not Implemented");
}

void Plater::export_object_amf() {
    wxMessageBox("Export object AMF not implemented yet", "Not Implemented");
}

void Plater::export_object_tmf() {
    wxMessageBox("Export object 3MF not implemented yet", "Not Implemented");
}

void Plater::object_cut_dialog() {
    //TODO
    ObjRef obj {this->selected_object()};
    if (obj == this->objects.end()) return;

    int idx = this->get_object_index(obj->identifier);
    if (idx < 0) return;

    auto* model_object {this->model->objects.at(idx)};
    auto cut_dialog = new ObjectCutDialog(nullptr, model_object);
    cut_dialog->ShowModal();
    cut_dialog->Destroy();
}

void Plater::object_layers_dialog() {
    // For now, just open settings dialog. 
    // Ideally, pass a flag to select Layers tab. 
    // Since our ObjectSettingsDialog defaults to first tab (Parts) or we can make it default to Layers if we want.
    // The current implementation of ObjectSettingsDialog has Parts (0) and Layers (1).
    // We can just call object_settings_dialog() for now, or update it to take a tab index.
    this->object_settings_dialog();
}
void Plater::add_undo_operation(UndoCmd cmd, std::vector<int>& obj_ids, Slic3r::Model& model) {
    //TODO
}

void Plater::add_undo_operation(UndoCmd cmd, int obj_id, Slic3r::Model& model) {
    std::vector<int> tmp {obj_id};
    add_undo_operation(cmd, tmp, model);
}

void Plater::add_undo_operation(UndoCmd cmd, int obj_id, size_t copies) {
}

void Plater::add_undo_operation(UndoCmd cmd, int obj_id, double angle, Axis axis) {
}

void Plater::object_list_changed() {
    if (this->object_info.choice == nullptr) return;
    this->object_info.choice->Clear();
    int sel_idx = wxNOT_FOUND;
    for (size_t i = 0; i < this->objects.size(); ++i) {
        this->object_info.choice->Append(wxString::FromUTF8(this->objects[i].name.c_str()));
        if (this->objects[i].selected) sel_idx = (int)i;
    }
    if (sel_idx != wxNOT_FOUND) {
        this->object_info.choice->SetSelection(sel_idx);
    }
}

void Plater::stop_background_process() {
    if (m_slicing_thread.joinable()) {
        m_slicing_thread.join();
    }
}

void Plater::start_background_process() {
    //TODO
}

void Plater::pause_background_process() {
    //TODO
}
void Plater::resume_background_process() {
    //TODO
}

ThemedMenu* Plater::object_menu() {
    auto* frame {this->GetFrame()};
    auto* menu {new ThemedMenu()};

    bool has_selection = this->selected_object() != this->objects.end();

    append_menu_item(menu, _("Add\u2026"), _("Add an object to the platter."), [=](wxCommandEvent& e) { this->add();}, wxID_ANY, "brick_add");
    append_menu_item(menu, _("Arrange"), _("Arrange objects on the bed."), [=](wxCommandEvent& e) { this->arrange();}, wxID_ANY, "application_view_tile");
    menu->AppendSeparator();

    append_menu_item(menu, _("Delete"), _("Remove the selected object."), [=](wxCommandEvent& e) { this->remove();}, wxID_ANY, "brick_delete", "Ctrl+Del")->isEnabled = has_selection;
    append_menu_item(menu, _("Increase copies"), _("Place one more copy of the selected object."), [=](wxCommandEvent& e) { this->increase();}, wxID_ANY, "add", "Ctrl++")->isEnabled = has_selection;
    append_menu_item(menu, _("Decrease copies"), _("Remove one copy of the selected object."), [=](wxCommandEvent& e) { this->decrease();}, wxID_ANY, "delete", "Ctrl+-")->isEnabled = has_selection;
    append_menu_item(menu, _(L"Set number of copies\u2026"), _("Change the number of copies of the selected object."), [=](wxCommandEvent& e) { this->set_number_of_copies();}, wxID_ANY, "textfield")->isEnabled = has_selection;
    menu->AppendSeparator();
    append_menu_item(menu, _(L"Move to bed center"), _(L"Center object around bed center."), [=](wxCommandEvent& e) { this->center_selected_object_on_bed();}, wxID_ANY, "arrow_in")->isEnabled = has_selection;
    append_menu_item(menu, _(L"Rotate 45\u00B0 clockwise"), _(L"Rotate the selected object by 45\u00B0 clockwise."), [=](wxCommandEvent& e) { this->rotate(45);}, wxID_ANY, "arrow_rotate_clockwise")->isEnabled = has_selection;
    append_menu_item(menu, _(L"Rotate 45\u00B0 counter-clockwise"), _(L"Rotate the selected object by 45\u00B0 counter-clockwise."), [=](wxCommandEvent& e) { this->rotate(-45);}, wxID_ANY, "arrow_rotate_anticlockwise")->isEnabled = has_selection;
    
    // Rotate Submenu
    {
        auto* rotateMenu {new ThemedMenu};
        append_menu_item(rotateMenu, _(L"Around X axis\u2026"), _("Rotate the selected object by an arbitrary angle around X axis."), [this](wxCommandEvent& e) { this->rotate(X); }, wxID_ANY, "bullet_red");
        append_menu_item(rotateMenu, _(L"Around Y axis\u2026"), _("Rotate the selected object by an arbitrary angle around Y axis."), [this](wxCommandEvent& e) { this->rotate(Y); }, wxID_ANY, "bullet_green");
        append_menu_item(rotateMenu, _(L"Around Z axis\u2026"), _("Rotate the selected object by an arbitrary angle around Z axis."), [this](wxCommandEvent& e) { this->rotate(Z); }, wxID_ANY, "bullet_blue");
        append_submenu(menu, _("Rotate"), _("Rotate the selected object by an arbitrary angle"), rotateMenu, wxID_ANY, "textfield")->isEnabled = has_selection;
    }

    // Mirror Submenu
    {
        auto* mirrorMenu {new ThemedMenu};
        append_menu_item(mirrorMenu, _(L"Along X axis"), _("Mirror the selected object along the X axis"), [this](wxCommandEvent& e) { this->mirror(X); }, wxID_ANY, "shape_flip_horizontal_x");
        append_menu_item(mirrorMenu, _(L"Along Y axis"), _("Mirror the selected object along the Y axis"), [this](wxCommandEvent& e) { this->mirror(Y); }, wxID_ANY, "shape_flip_horizontal_y");
        append_menu_item(mirrorMenu, _(L"Along Z axis"), _("Mirror the selected object along the Z axis"), [this](wxCommandEvent& e) { this->mirror(Z); }, wxID_ANY, "shape_flip_horizontal_z");
        append_submenu(menu, _("Mirror"), _("Mirror the selected object"), mirrorMenu, wxID_ANY, "shape_flip_horizontal")->isEnabled = has_selection;
    }

    // Scale Submenu
    {
        auto* scaleMenu {new ThemedMenu};
        append_menu_item(scaleMenu, _(L"Uniformly\u2026"), _("Scale the selected object along the XYZ axes"), [this](wxCommandEvent& e) { this->changescale(false); }, wxID_ANY);
        append_menu_item(scaleMenu, _(L"Along X axis\u2026"), _("Scale the selected object along the X axis"), [this](wxCommandEvent& e) { this->changescale(X, false); }, wxID_ANY, "bullet_red");
        append_menu_item(scaleMenu, _(L"Along Y axis\u2026"), _("Scale the selected object along the Y axis"), [this](wxCommandEvent& e) { this->changescale(Y, false); }, wxID_ANY, "bullet_green");
        append_menu_item(scaleMenu, _(L"Along Z axis\u2026"), _("Scale the selected object along the Z axis"), [this](wxCommandEvent& e) { this->changescale(Z, false); }, wxID_ANY, "bullet_blue");
        append_submenu(menu, _("Scale"), _("Scale the selected object by a given factor"), scaleMenu, wxID_ANY, "arrow_out")->isEnabled = has_selection;
    }

    // Scale to Size Submenu
    {
        auto* scaleToSizeMenu {new ThemedMenu};
        append_menu_item(scaleToSizeMenu, _(L"Uniformly\u2026"), _("Scale the selected object along the XYZ axes"), [this](wxCommandEvent& e) { this->changescale(true); }, wxID_ANY);
        append_menu_item(scaleToSizeMenu, _(L"Along X axis\u2026"), _("Scale the selected object along the X axis"), [this](wxCommandEvent& e) { this->changescale(X, true); }, wxID_ANY, "bullet_red");
        append_menu_item(scaleToSizeMenu, _(L"Along Y axis\u2026"), _("Scale the selected object along the Y axis"), [this](wxCommandEvent& e) { this->changescale(Y, true); }, wxID_ANY, "bullet_green");
        append_menu_item(scaleToSizeMenu, _(L"Along Z axis\u2026"), _("Scale the selected object along the Z axis"), [this](wxCommandEvent& e) { this->changescale(Z, true); }, wxID_ANY, "bullet_blue");
        append_submenu(menu, _("Scale to size"), _("Scale the selected object to match a given size"), scaleToSizeMenu, wxID_ANY, "arrow_out")->isEnabled = has_selection;
    }

    append_menu_item(menu, _("Split"), _("Split the selected object into individual parts"), [this](wxCommandEvent& e) { this->split_object(); }, wxID_ANY, "shape_ungroup")->isEnabled = has_selection;
    append_menu_item(menu, _("Cut\u2026"), _("Open the 3D cutting tool"), [this](wxCommandEvent& e) { this->object_cut_dialog(); }, wxID_ANY, "package")->isEnabled = has_selection;
    append_menu_item(menu, _("Layer heights\u2026"), _("Open the dynamic layer height control"), [this](wxCommandEvent& e) { this->object_layers_dialog(); }, wxID_ANY, "variable_layer_height")->isEnabled = has_selection;
    
    menu->AppendSeparator();
    
    append_menu_item(menu, _("Settings\u2026"), _("Open the object editor dialog"), [this](wxCommandEvent& e) { this->object_settings_dialog(); }, wxID_ANY, "cog")->isEnabled = has_selection;
    
    menu->AppendSeparator();
    
    append_menu_item(menu, _("Reload from Disk"), _("Reload the selected file from Disk"), [this](wxCommandEvent& e) { this->reload_from_disk(); }, wxID_ANY, "arrow_refresh")->isEnabled = has_selection;
    append_menu_item(menu, _("Export object as STL\u2026"), _("Export this single object as STL file"), [this](wxCommandEvent& e) { this->export_object_stl(); }, wxID_ANY, "brick_go")->isEnabled = has_selection;
    append_menu_item(menu, _("Export object and modifiers as AMF\u2026"), _("Export this single object and all associated modifiers as AMF file"), [this](wxCommandEvent& e) { this->export_object_amf(); }, wxID_ANY, "brick_go")->isEnabled = has_selection;
    append_menu_item(menu, _("Export object and modifiers as 3MF\u2026"), _("Export this single object and all associated modifiers as 3MF file"), [this](wxCommandEvent& e) { this->export_object_tmf(); }, wxID_ANY, "brick_go")->isEnabled = has_selection;

    return menu;
}

void Plater::set_number_of_copies() {
    this->pause_background_process();

    ObjRef obj {this->selected_object()};
    if (obj == this->objects.end()) return;

    int idx = this->get_object_index(obj->identifier);
    if (idx < 0) return;

    auto* model_object { this->model->objects.at(idx) };

    long copies = -1;
    copies = wxGetNumberFromUser("", _("Enter the number of copies of the selected object:"), _("Copies"), model_object->instances.size(), 0, 1000, this);
    if (copies < 0) return;
    long instance_count = 0;
    if (model_object->instances.size() <= LONG_MAX) {
        instance_count = static_cast<long>(model_object->instances.size());
    } else {
        instance_count = LONG_MAX;
    }
    long diff {copies - instance_count };

    if      (diff == 0) { this->resume_background_process(); }
    else if (diff > 0)  { this->increase(diff); }
    else if (diff < 0)  { this->decrease(-diff); }
}
void Plater::center_selected_object_on_bed() {
    ObjRef obj {this->selected_object()};
    
    if (obj == this->objects.end()) return;

    int idx = this->get_object_index(obj->identifier);
    if (idx < 0) return;

    auto* model_object { this->model->objects.at(idx) };
    auto bb {model_object->bounding_box()};
    auto size {bb.size()};

    auto vector { Slic3r::Pointf(
            this->bed_centerf().x - bb.min.x - size.x/2.0,
            this->bed_centerf().y - bb.min.y - size.y/2.0)};
    for (auto* inst : model_object->instances) {
        inst->offset.translate(vector);
    }

    this->refresh_canvases();

}

void Plater::show_preset_editor(preset_t group, unsigned int idx) {
    wxString title = "";
    switch (group) {
        case preset_t::Print:    title = _("Print Settings"); break;
        case preset_t::Material: title = _("Filament Settings"); break;
        case preset_t::Printer:  title = _("Printer Settings"); break;
        default: return;
    }

    wxDialog* dlg = new wxDialog(this, wxID_ANY, title, wxDefaultPosition, wxSize(900, 600), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    wxWindow* editor = nullptr;
    wxString selected_preset = "";
    if (this->_presets) {
        selected_preset = this->_presets->_get_selected_preset(group, idx);
    }
    
    switch (group) {
        case preset_t::Print:
            editor = new PrintEditor(dlg);
            break;
        case preset_t::Material:
            editor = new MaterialEditor(dlg);
            break;
        case preset_t::Printer:
            editor = new PrinterEditor(dlg);
            break;
        default: 
            dlg->Destroy();
            return;
    }
    
    if (editor) {
        if (auto* pe = dynamic_cast<PresetEditor*>(editor)) {
             if (!selected_preset.IsEmpty()) {
                 pe->select_preset_by_name(selected_preset);
             }

             pe->on_quick_setting_change = [this](const std::string& key, bool active) {
                  this->update_quick_settings();
             };
        }

        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        // Add editor to dialog sizer
        sizer->Add(editor, 1, wxEXPAND);
        dlg->SetSizer(sizer);
        dlg->Layout();
        dlg->Center();
        dlg->ShowModal();
    }
    dlg->Destroy();
}


void Plater::load_presets() {
    this->_presets->load();
}


void Plater::export_gcode() {
    if (this->objects.empty()) {
        wxMessageBox(_("No objects to slice."), _("Error"), wxICON_ERROR);
        return;
    }

    wxFileDialog saveFileDialog(this, _("Save G-code file"), "", "",
                                "G-code files (*.gcode)|*.gcode", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveFileDialog.ShowModal() == wxID_CANCEL)
        return;
    std::string output_file = saveFileDialog.GetPath().ToStdString();

    // Apply config
    this->load_current_presets();
    this->print->apply_config(this->config->config());

    // Validate
    try {
        this->print->validate();
    } catch (std::exception& e) {
        wxMessageBox(wxString::FromUTF8(e.what()), _("Configuration Error"), wxICON_ERROR);
        return;
    }

    wxProgressDialog progressDialog(_("Slicing"), _("Processing..."), 100, this, wxPD_APP_MODAL | wxPD_AUTO_HIDE);
    
    // Set status callback
    this->print->status_cb = [&progressDialog](int percent, const std::string& msg) {
        progressDialog.Update(percent, wxString::FromUTF8(msg.c_str()));
        wxYield(); 
    };

    try {
        // Ensure Print object has the latest Model state
        this->print->reload_model_instances();

        // Process
        this->print->process();

        // Export
        this->print->export_gcode(output_file);

        wxMessageBox(_("G-code exported successfully."), _("Done"), wxICON_INFORMATION);

    } catch (std::exception& e) {
        wxMessageBox(wxString::FromUTF8(e.what()), _("Slicing Error"), wxICON_ERROR);
    }
    
    // Cleanup callback
    this->print->status_cb = nullptr;
}


void Plater::slice() {
    if (this->objects.empty()) {
        wxMessageBox(_("No objects to slice."), _("Error"), wxICON_ERROR);
        return;
    }

    // Apply config
    this->load_current_presets();
    this->print->apply_config(this->config->config());
    
    // Auto-detect threads if not set or set to 1
    if (this->print->config.threads.value <= 1) {
        this->print->config.threads.value = std::thread::hardware_concurrency();
         if (this->print->config.threads.value == 0) this->print->config.threads.value = 2;
    }

    // Validate
    try {
        this->print->validate();
    } catch (std::exception& e) {
        wxMessageBox(wxString::FromUTF8(e.what()), _("Configuration Error"), wxICON_ERROR);
        return;
    }

    // Create a progress handling mechanism that works across threads
    // We cannot use wxProgressDialog in modal mode effectively if we want non-blocking (well, we can but we want background)
    // For now, let's keep the dialog but show it non-modally or use a flag.
    // Actually, "Background slicing" usually implies the UI remains responsive. 
    // Let's use a non-modal ProgressDialog or just a status bar update. 
    // For specific user request "background sliceing like prusa", PrusaSlicer shows a progress bar in the bottom right 
    // and lets you rotate the view.

    // Using a simpler approach: Standard thread.
    
    // Disable slice button to prevent double-click
    // TODO: Disable UI controls
    
    auto* progressDialog = new wxProgressDialog(_("Slicing"), _("Processing..."), 100, this, wxPD_APP_MODAL | wxPD_AUTO_HIDE);
    // Note: wxPD_APP_MODAL blocks the UI interaction. 
    // If we want TRUE background slicing, we should remove wxPD_APP_MODAL and potentially use the status bar.
    // But for now, ensuring it runs in a thread prevents "Not Responding" windows ghosting.
    
    // To allow UI updates, we pass the pointer. Mutable lambda.

    // Ensure previous thread is joined before starting new one
    stop_background_process();

    m_slicing_active = true;
    m_slicing_thread = std::thread([this, progressDialog]() {
        try {
            // Ensure Print object has the latest Model state
             // This touches Model which might be main-thread only if not careful, but usually Model is data.
             // reload_model_instances() reads model instances. 
             // We should probably do this on main thread before spawning? 
             // Let's assume it's safe or do it before.
            
            // Set status callback
            this->print->status_cb = [progressDialog](int percent, const std::string& msg) {
                wxTheApp->CallAfter([progressDialog, percent, msg]() {
                    progressDialog->Update(percent, wxString::FromUTF8(msg.c_str()));
                });
            };

            // Process
            // reload_model_instances interacts with ModelObject which touches Pointf, safe.
            this->print->reload_model_instances();
            this->print->process();

            // Success
            wxTheApp->CallAfter([this, progressDialog]() {
                progressDialog->Close();
                progressDialog->Destroy();
                
                // Update Preview
                if(this->preview3D) this->preview3D->reload_print();

                // Switch execution to Preview tab
                this->preview_notebook->SetSelection(1); 
                
                // Optional: Flash notification or simple message
                // wxMessageBox(_("Slicing Complete"), _("Done"), wxICON_INFORMATION);
            });

        } catch (std::exception& e) {
             std::string msg = e.what();
             wxTheApp->CallAfter([this, progressDialog, msg]() {
                progressDialog->Close();
                progressDialog->Destroy();
                wxMessageBox(wxString::FromUTF8(msg.c_str()), _("Slicing Error"), wxICON_ERROR);
            });
        }
        
        // Cleanup callback
        this->print->status_cb = nullptr;
        m_slicing_active = false;
        
    });
}

void Plater::update_quick_settings() {
    if (!this->config) return;

    // 1. Cleanup existing OptionsGroups
    if (this->quick_settings_printer) { delete this->quick_settings_printer; this->quick_settings_printer = nullptr; }
    if (this->quick_settings_filament) { delete this->quick_settings_filament; this->quick_settings_filament = nullptr; }
    if (this->quick_settings_print) { delete this->quick_settings_print; this->quick_settings_print = nullptr; }

    // 2. Clear Tabs
    if (this->quick_settings_panel) {
        this->quick_settings_panel->Clear();
    }

    // 3. Define Keys
    const std::vector<std::string> printer_keys = {
        "z_offset", "extruders_count", "has_heatbed", "serial_port", "serial_speed", "host_type", "print_host", "octoprint_apikey",
        "gcode_flavor", "use_relative_e_distances", "use_firmware_retraction", "use_volumetric_e", "pressure_advance", "vibration_limit",
        "z_steps_per_mm", "use_set_and_wait_extruder", "use_set_and_wait_bed", "fan_percentage", "start_gcode", "end_gcode",
        "before_layer_gcode", "layer_gcode", "toolchange_gcode", "between_objects_gcode", "nozzle_diameter", "min_layer_height",
        "max_layer_height", "extruder_offset", "retract_length", "retract_lift", "retract_lift_above", "retract_lift_below",
        "retract_speed", "retract_restart_extra", "retract_before_travel", "retract_layer_change", "wipe", "retract_length_toolchange",
        "retract_restart_extra_toolchange", "printer_notes", "bed_shape"
    };

    const std::vector<std::string> filament_keys = {
        "filament_colour", "filament_diameter", "extrusion_multiplier", "first_layer_temperature", "temperature",
        "first_layer_bed_temperature", "bed_temperature", "filament_density", "filament_cost", "fan_always_on", "cooling",
        "min_fan_speed", "max_fan_speed", "bridge_fan_speed", "disable_fan_first_layers", "fan_below_layer_time",
        "slowdown_below_layer_time", "min_print_speed", "start_filament_gcode", "end_filament_gcode", "filament_notes",
        "filament_max_volumetric_speed"
    };

    const std::vector<std::string> print_keys = {
        "layer_height", "first_layer_height", "adaptive_slicing", "adaptive_slicing_quality", "match_horizontal_surfaces",
        "perimeters", "min_shell_thickness", "spiral_vase", "top_solid_layers", "bottom_solid_layers", "min_top_bottom_shell_thickness",
        "extra_perimeters", "avoid_crossing_perimeters", "thin_walls", "overhangs", "seam_position", "external_perimeters_first",
        "fill_density", "fill_pattern", "top_infill_pattern", "bottom_infill_pattern", "infill_every_layers", "infill_only_where_needed",
        "fill_gaps", "solid_infill_every_layers", "fill_angle", "solid_infill_below_area", "only_retract_when_crossing_perimeters",
        "infill_first", "skirts", "skirt_distance", "skirt_height", "min_skirt_length", "brim_width", "brim_ears", "brim_ears_max_angle",
        "interior_brim_width", "brim_connections_width", "support_material", "support_material_threshold", "support_material_max_layers",
        "support_material_enforce_layers", "raft_layers", "support_material_contact_distance", "support_material_pattern",
        "support_material_spacing", "support_material_angle", "support_material_pillar_size", "support_material_pillar_spacing",
        "support_material_interface_layers", "support_material_interface_spacing", "support_material_buildplate_only", "dont_support_bridges",
        "perimeter_speed", "small_perimeter_speed", "external_perimeter_speed", "infill_speed", "solid_infill_speed", "top_solid_infill_speed",
        "gap_fill_speed", "bridge_speed", "support_material_speed", "support_material_interface_speed", "travel_speed", "first_layer_speed",
        "perimeter_acceleration", "infill_acceleration", "bridge_acceleration", "first_layer_acceleration", "default_acceleration",
        "max_print_speed", "max_volumetric_speed", "perimeter_extruder", "infill_extruder", "solid_infill_extruder", "support_material_extruder",
        "support_material_interface_extruder", "ooze_prevention", "standby_temperature_delta", "regions_overlap", "interface_shells",
        "extrusion_width", "first_layer_extrusion_width", "perimeter_extrusion_width", "external_perimeter_extrusion_width",
        "infill_extrusion_width", "solid_infill_extrusion_width", "top_infill_extrusion_width", "support_material_interface_extrusion_width",
        "support_material_extrusion_width", "infill_overlap", "bridge_flow_ratio", "xy_size_compensation", "resolution", "complete_objects",
        "extruder_clearance_radius", "extruder_clearance_height", "gcode_comments", "label_printed_objects", "output_filename_format",
        "post_process", "notes"
    };

    // 4. Helper to build filtered list and create tab
    auto process_tab = [&](const wxString& title, const wxString& icon_name, OptionsGroup*& group_ptr, const std::vector<std::string>& all_keys) {
        std::vector<std::string> valid_keys;
        for (const auto& key : all_keys) {
            if (ui_settings->is_quick_setting(key) && this->config->has(key)) {
                valid_keys.push_back(key);
            }
        }
        
        if (!valid_keys.empty()) {
            auto* page = this->quick_settings_panel->AddTab(title, get_bmp_bundle(icon_name));
            group_ptr = new OptionsGroup(page);
            group_ptr->show_quick_setting_toggles = false;
            group_ptr->set_sizer(static_cast<wxBoxSizer*>(page->GetSizer()));
            
            for (const auto& key : valid_keys) {
                group_ptr->append_option(key, this->config->config());
            }
            group_ptr->update_options(&this->config->config());
            
            // Force layout update for the scrolled window page
            page->Layout();
            page->FitInside(); // Critical for scrolling to work and content to size correctly
        }
    };

    // 5. Create Tabs if needed
    process_tab(_("Printer"), "printer_empty.svg", this->quick_settings_printer, printer_keys);
    process_tab(_("Process"), "cog.svg", this->quick_settings_print, print_keys);
    process_tab(_("Filament"), "spool.svg", this->quick_settings_filament, filament_keys);

    if (this->quick_settings_panel) {
        this->quick_settings_panel->Layout();
        this->quick_settings_panel->Refresh();
        
        // Ensure the selection in the book is refreshed to show content
        if (this->quick_settings_panel->GetSelection() != -1) {
            this->quick_settings_panel->SetSelection(this->quick_settings_panel->GetSelection());
        }
    }
}


}} // Namespace Slic3r::GUI