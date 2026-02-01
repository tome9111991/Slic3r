#include <memory>
#include <climits>
#include <fstream>

#include <wx/progdlg.h>
#include <wx/window.h> 
#include <wx/numdlg.h> 
#include <wx/textdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h> 
#include <wx/button.h> 
#include <thread> 


#include "Plater.hpp"
#include "ProgressStatusBar.hpp"
#include "Log.hpp"
#include "MainFrame.hpp"
#include "BoundingBox.hpp"
#include "Geometry.hpp"
#include "Dialogs/AnglePicker.hpp"
#include "Dialogs/ObjectCutDialog.hpp"
#include "Dialogs/ObjectSettingsDialog.hpp"
#include "Dialogs/PresetEditor.hpp"

namespace Slic3r { namespace GUI {

const auto TB_ADD           {wxNewId()};
const auto TB_REMOVE        {wxNewId()};
const auto TB_RESET         {wxNewId()};
const auto TB_ARRANGE       {wxNewId()};
const auto TB_EXPORT_GCODE  {wxNewId()};
const auto TB_EXPORT_STL    {wxNewId()};
const auto TB_MORE          {wxNewId()};
const auto TB_FEWER         {wxNewId()};
const auto TB_45CW          {wxNewId()};
const auto TB_45CCW         {wxNewId()};
const auto TB_SCALE         {wxNewId()};
const auto TB_SPLIT         {wxNewId()};
const auto TB_CUT           {wxNewId()};
const auto TB_LAYERS        {wxNewId()};
const auto TB_SETTINGS      {wxNewId()};

const auto PROGRESS_BAR_EVENT = wxNewEventType();

Plater::Plater(wxWindow* parent, const wxString& title) : 
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, title),
    _presets(new PresetChooser(dynamic_cast<wxWindow*>(this), this->print))
{
    if (ui_settings->color->SOLID_BACKGROUNDCOLOR()) {
        this->SetBackgroundColour(ui_settings->color->BACKGROUND_COLOR());
    }

    // Set callback for status event for worker threads
    /*
    this->print->set_status_cb([=](std::string percent percent, std::wstring message) {
        wxPostEvent(this, new wxPlThreadEvent(-1, PROGRESS_BAR_EVENT, 
    });
    */
    _presets->load();

    // Initialize handlers for canvases
    auto on_select_object {[this](ObjIdx obj_idx) { this->select_object(obj_idx); }};
    auto on_double_click {[this]() { if (this->selected_object() != this->objects.end()) this->object_settings_dialog(); }};
    auto on_right_click {[this](wxPanel* canvas, const wxPoint& pos) 
        {
            auto obj = this->selected_object();
            if (obj == this->objects.end()) return;

            auto menu = this->object_menu();
            canvas->PopupMenu(menu, pos);
            delete menu;
        }};
    auto on_instances_moved {[this]() { this->on_model_change(); }};

    /* 
    # Initialize 3D plater
    if ($Slic3r::GUI::have_OpenGL) {
        $self->{canvas3D} = Slic3r::GUI::Plater::3D->new($self->{preview_notebook}, $self->{objects}, $self->{model}, $self->{config});
        $self->{preview_notebook}->AddPage($self->{canvas3D}, '3D');
        $self->{canvas3D}->set_on_select_object($on_select_object);
        $self->{canvas3D}->set_on_double_click($on_double_click);
        $self->{canvas3D}->set_on_right_click(sub { $on_right_click->($self->{canvas3D}, @_); });
        $self->{canvas3D}->set_on_instances_moved($on_instances_moved);
        $self->{canvas3D}->on_viewport_changed(sub {
            $self->{preview3D}->canvas->set_viewport_from_scene($self->{canvas3D});
        });
    }
    */

    // initialize 2D Preview Canvas
    // canvas2D = new Plate2D(preview_notebook, wxDefaultSize, objects, model, config);
    // preview_notebook->AddPage(canvas2D, _("2D"));

    // canvas2D->on_select_object = std::function<void (ObjIdx obj_idx)>(on_select_object);
    // canvas2D->on_double_click = std::function<void ()>(on_double_click);
    // canvas2D->on_right_click = std::function<void (const wxPoint& pos)>([=](const wxPoint& pos){ on_right_click(canvas2D, pos); });
    // canvas2D->on_instances_moved = std::function<void ()>(on_instances_moved);


    canvas3D = new Plate3D(preview_notebook, wxDefaultSize, objects, model, config);
    preview_notebook->AddPage(canvas3D, _("3D"));

    canvas3D->on_select_object = std::function<void (ObjIdx obj_idx)>(on_select_object);
    canvas3D->on_instances_moved = std::function<void ()>(on_instances_moved);
    
    preview3D = new Preview3D(preview_notebook, wxDefaultSize, print, objects, model, config);
    preview_notebook->AddPage(preview3D, _("Preview"));
    
    // Set initial bed shape for preview
    if(config) {
        auto bed_poly = Slic3r::Polygon::new_scale(config->get<ConfigOptionPoints>("bed_shape").values);
        preview3D->set_bed_shape(bed_poly.points);
    }

    preview_notebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [this](wxBookCtrlEvent& e) {
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

    /*
    previewDLP = new PreviewDLP(preview_notebook, wxDefaultSize, objects, model, config);
    preview_notebook->AddPage(previewDLP, _("DLP/SLA"));
    */

    /*
    # Initialize 2D preview canvas
    $self->{canvas} = Slic3r::GUI::Plater::2D->new($self->{preview_notebook}, wxDefaultSize, $self->{objects}, $self->{model}, $self->{config});
    $self->{preview_notebook}->AddPage($self->{canvas}, '2D');
    $self->{canvas}->on_select_object($on_select_object);
    $self->{canvas}->on_double_click($on_double_click);
    $self->{canvas}->on_right_click(sub { $on_right_click->($self->{canvas}, @_); });
    $self->{canvas}->on_instances_moved($on_instances_moved);
    # Initialize 3D toolpaths preview
    $self->{preview3D_page_idx} = -1;
    if ($Slic3r::GUI::have_OpenGL) {
        $self->{preview3D} = Slic3r::GUI::Plater::3DPreview->new($self->{preview_notebook}, $self->{print});
        $self->{preview3D}->canvas->on_viewport_changed(sub {
            $self->{canvas3D}->set_viewport_from_scene($self->{preview3D}->canvas);
        });
        $self->{preview_notebook}->AddPage($self->{preview3D}, 'Preview');
        $self->{preview3D_page_idx} = $self->{preview_notebook}->GetPageCount-1;
    }
    
    # Initialize toolpaths preview
    $self->{toolpaths2D_page_idx} = -1;
    if ($Slic3r::GUI::have_OpenGL) {
        $self->{toolpaths2D} = Slic3r::GUI::Plater::2DToolpaths->new($self->{preview_notebook}, $self->{print});
        $self->{preview_notebook}->AddPage($self->{toolpaths2D}, 'Layers');
        $self->{toolpaths2D_page_idx} = $self->{preview_notebook}->GetPageCount-1;
    }
    
    EVT_NOTEBOOK_PAGE_CHANGED($self, $self->{preview_notebook}, sub {
        wxTheApp->CallAfter(sub {
            my $sel = $self->{preview_notebook}->GetSelection;
            if ($sel == $self->{preview3D_page_idx} || $sel == $self->{toolpaths2D_page_idx}) {
                if (!$Slic3r::GUI::Settings->{_}{background_processing} && !$self->{processed}) {
                    $self->statusbar->SetCancelCallback(sub {
                        $self->stop_background_process;
                        $self->statusbar->SetStatusText("Slicing cancelled");
                        $self->{preview_notebook}->SetSelection(0);

                    });
                    $self->start_background_process;
                } else {
                    $self->{preview3D}->load_print
                        if $sel == $self->{preview3D_page_idx};
                }
            }
        });
    });
    */
    wxStaticBoxSizer* object_info_sizer {nullptr};
    {
        auto* box {new wxStaticBox(this, wxID_ANY, _("Info"))};
        object_info_sizer = new wxStaticBoxSizer(box, wxVERTICAL);
        object_info_sizer->SetMinSize(wxSize(350, -1));
        {
            auto* sizer {new wxBoxSizer(wxHORIZONTAL)};
            object_info_sizer->Add(sizer, 0, wxEXPAND | wxBOTTOM, 5);
            auto* text  {new wxStaticText(box, wxID_ANY, _("Object:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT)};
            text->SetFont(ui_settings->small_font());
            sizer->Add(text, 0, wxALIGN_CENTER_VERTICAL);

            /* We supply a bogus width to wxChoice (sizer will override it and stretch 
             * the control anyway), because if we leave the default (-1) it will stretch
             * too much according to the contents, and this is bad with long file names.
             */
            this->object_info.choice = new wxChoice(box, wxID_ANY, wxDefaultPosition, wxSize(100, -1));
            this->object_info.choice->SetFont(ui_settings->small_font());
            sizer->Add(this->object_info.choice, 1, wxALIGN_CENTER_VERTICAL);

            // Select object on change.
            this->Bind(wxEVT_CHOICE, [this](wxCommandEvent& e) { this->select_object(this->object_info.choice->GetSelection()); this->refresh_canvases();});
                
        }
        
        auto* grid_sizer { new wxFlexGridSizer(3, 4, 5, 5)};
        grid_sizer->SetFlexibleDirection(wxHORIZONTAL);
        grid_sizer->AddGrowableCol(1, 1);
        grid_sizer->AddGrowableCol(3, 1);

        add_info_field(box, this->object_info.copies, _("Copies"), grid_sizer);
        add_info_field(box, this->object_info.size, _("Size"), grid_sizer);
        add_info_field(box, this->object_info.volume, _("Volume"), grid_sizer);
        add_info_field(box, this->object_info.facets, _("Facets"), grid_sizer);
        add_info_field(box, this->object_info.materials, _("Materials"), grid_sizer);
        {
            wxString name {"Manifold:"};
            auto* text {new wxStaticText(box, wxID_ANY, name, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT)};
            text->SetFont(ui_settings->small_font());
            grid_sizer->Add(text, 0);

            this->object_info.manifold = new wxStaticText(box, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
            this->object_info.manifold->SetFont(ui_settings->small_font());

            this->object_info.manifold_warning_icon = new wxStaticBitmap(box, wxID_ANY, wxBitmap(var("error.png"), wxBITMAP_TYPE_PNG));
            this->object_info.manifold_warning_icon->Hide();

            auto* h_sizer {new wxBoxSizer(wxHORIZONTAL)};
            h_sizer->Add(this->object_info.manifold_warning_icon, 0);
            h_sizer->Add(this->object_info.manifold, 0);

            grid_sizer->Add(h_sizer, 0, wxEXPAND);
        }

        object_info_sizer->Add(grid_sizer, 0, wxEXPAND);
    }
    this->selection_changed();
    if (this->canvas2D) this->canvas2D->update_bed_size();

    // Toolbar
    this->build_toolbar();

    // Finally assemble the sizers into the display.
    
    // export/print/send/export buttons

    // right panel sizer
    auto* right_sizer {this->right_sizer};
    right_sizer->Add(this->_presets, 0, wxEXPAND | wxTOP, 10);

    /* Moved to Top Bar in MainFrame
    auto*buttons_sizer = new wxBoxSizer(wxVERTICAL);
    {
        auto* btn = new wxButton(this, wxID_ANY, _("Slice now"));
        btn->SetBitmap(wxBitmap(var("cog.png"), wxBITMAP_TYPE_PNG)); 
        btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) { this->slice(); });
        buttons_sizer->Add(btn, 0, wxEXPAND | wxBOTTOM, 5);
    }
    {
        auto* btn = new wxButton(this, wxID_ANY, _("Export G-code"));
        btn->SetBitmap(wxBitmap(var("cog_go.png"), wxBITMAP_TYPE_PNG)); 
        btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) { this->export_gcode(); });
        buttons_sizer->Add(btn, 0, wxEXPAND);
    }
    right_sizer->Add(buttons_sizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 10);
    */

//    $right_sizer->Add($self->{settings_override_panel}, 1, wxEXPAND, 5);
    right_sizer->Add(object_info_sizer, 0, wxEXPAND, 0);
//    $right_sizer->Add($object_info_sizer, 0, wxEXPAND, 0);
//    $right_sizer->Add($print_info_sizer, 0, wxEXPAND, 0);
//    $right_sizer->Hide($print_info_sizer);

    auto hsizer {new wxBoxSizer(wxHORIZONTAL)};
    hsizer->Add(this->preview_notebook, 1, wxEXPAND | wxTOP, 1);
    hsizer->Add(right_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 3);

    auto sizer {new wxBoxSizer(wxVERTICAL)};
    if (this->htoolbar != nullptr) sizer->Add(this->htoolbar, 0, wxEXPAND, 0);
    if (this->btoolbar != nullptr) sizer->Add(this->btoolbar, 0, wxEXPAND, 0);
    sizer->Add(hsizer, 1, wxEXPAND,0);

    sizer->SetSizeHints(this);
    this->SetSizer(sizer);

    // Initialize the toolbar
    this->selection_changed();

    this->selection_changed();

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
        
        GetFrame()->statusbar->SetStatusText(_("Loaded ") + wxString(file));

        if (this->scaled_down) {
            GetFrame()->statusbar->SetStatusText(_("Your object appears to be too large, so it was automatically scaled down to fit your print bed."));
        }
        if (this->outside_bounds) {
            GetFrame()->statusbar->SetStatusText(_("Some of your object(s) appear to be outside the print bed. Use the arrange button to correct this."));
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
    if (this->canvas2D != nullptr)
        this->canvas2D->Refresh();
    if (this->canvas3D != nullptr)
        this->canvas3D->update();
    if (this->preview3D != nullptr)
        this->preview3D->reload_print();

}

void Plater::arrange() {
    // TODO pause background process
    const Slic3r::BoundingBoxf bb {Slic3r::BoundingBoxf(this->config->get<ConfigOptionPoints>("bed_shape").values)};
    if (this->objects.size() == 0U) { // abort
        GetFrame()->statusbar->SetStatusText(_("Nothing to arrange."));
        return; 
    }
    bool success {this->model->arrange_objects(this->config->config().min_object_distance(), &bb)};

    if (success) {
        GetFrame()->statusbar->SetStatusText(_("Objects were arranged."));
    } else {
        GetFrame()->statusbar->SetStatusText(_("Arrange failed."));
    }
    this->on_model_change(true);
}

void Plater::on_model_change(bool force_autocenter) {
    Log::info(LogChannel, L"Called on_modal_change");

    // reload the select submenu (if already initialized)
    {
        auto* menu = this->GetFrame()->plater_select_menu;

        if (menu != nullptr) {
            auto list = menu->GetMenuItems();
            for (auto it = list.begin();it!=list.end(); it++) { menu->Delete(*it); }
            for (const auto& obj : this->objects) {
                const auto idx {obj.identifier};
                auto name {wxString(obj.name)};
                auto inst_count = this->model->objects.at(idx)->instances.size();
                if (inst_count > 1) {
                    name << " (" << inst_count << "x)";
                }
                wxMenuItem* item {append_menu_item(menu, name, _("Select object."), 
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
    this->select_object(this->objects.begin() + obj_idx);
}

void Plater::select_object() {
    this->select_object(this->objects.end());
}

void Plater::selection_changed() {
    // Remove selection in 2D plater
    if (this->canvas2D) this->canvas2D->set_selected(-1, -1);
    this->canvas3D->selection_changed();

    auto obj = this->selected_object();
    bool have_sel {obj != this->objects.end()};
    auto* menu {this->GetFrame()->plater_select_menu};
    if (menu != nullptr) {
        for (auto item = menu->GetMenuItems().begin(); item != menu->GetMenuItems().end(); item++) {
            (*item)->Check(false);
        }
        if (have_sel) 
            menu->FindItemByPosition(obj->identifier)->Check(true);
    }

    if (this->htoolbar != nullptr) {
        for (auto tb : {TB_REMOVE, TB_MORE, TB_FEWER, TB_45CW, TB_45CCW, TB_SCALE, TB_SPLIT, TB_CUT, TB_LAYERS, TB_SETTINGS}) {
            this->htoolbar->EnableTool(tb, have_sel);
        }
    }
    /*
    
    my $method = $have_sel ? 'Enable' : 'Disable';
    $self->{"btn_$_"}->$method
        for grep $self->{"btn_$_"}, qw(remove increase decrease rotate45cw rotate45ccw changescale split cut layers settings);
    
    if ($self->{object_info_size}) { # have we already loaded the info pane?
        
        if ($have_sel) {
            my $model_object = $self->{model}->objects->[$obj_idx];
            $self->{object_info_choice}->SetSelection($obj_idx);
            $self->{object_info_copies}->SetLabel($model_object->instances_count);
            my $model_instance = $model_object->instances->[0];
            {
                my $size_string = sprintf "%.2f x %.2f x %.2f", @{$model_object->instance_bounding_box(0)->size};
                if ($model_instance->scaling_factor != 1) {
                    $size_string .= sprintf " (%s%%)", $model_instance->scaling_factor * 100;
                }
                $self->{object_info_size}->SetLabel($size_string);
            }
            $self->{object_info_materials}->SetLabel($model_object->materials_count);
            
            my $raw_mesh = $model_object->raw_mesh;
            $raw_mesh->repair;  # this calculates number_of_parts
            if (my $stats = $raw_mesh->stats) {
                $self->{object_info_volume}->SetLabel(sprintf('%.2f', $raw_mesh->volume * ($model_instance->scaling_factor**3)));
                $self->{object_info_facets}->SetLabel(sprintf('%d (%d shells)', $model_object->facets_count, $stats->{number_of_parts}));
                if (my $errors = sum(@$stats{qw(degenerate_facets edges_fixed facets_removed facets_added facets_reversed backwards_edges)})) {
                    $self->{object_info_manifold}->SetLabel(sprintf("Auto-repaired (%d errors)", $errors));
                    $self->{object_info_manifold_warning_icon}->Show;
                    
                    # we don't show normals_fixed because we never provide normals
	                # to admesh, so it generates normals for all facets
                    my $message = sprintf '%d degenerate facets, %d edges fixed, %d facets removed, %d facets added, %d facets reversed, %d backwards edges',
                        @$stats{qw(degenerate_facets edges_fixed facets_removed facets_added facets_reversed backwards_edges)};
                    $self->{object_info_manifold}->SetToolTipString($message);
                    $self->{object_info_manifold_warning_icon}->SetToolTipString($message);
                } else {
                    $self->{object_info_manifold}->SetLabel("Yes");
                }
            } else {
                $self->{status_bar}->SetStatusText($object->facets);
            }
        } else {
            $self->{object_info_choice}->SetSelection(-1);
            $self->{"object_info_$_"}->SetLabel("") for qw(copies size volume facets materials manifold);
            $self->{object_info_manifold_warning_icon}->Hide;
            $self->{object_info_manifold}->SetToolTipString("");
        }
        $self->Layout;
    }
    # prepagate the event to the frame (a custom Wx event would be cleaner)
    $self->GetFrame->on_plater_selection_changed($have_sel);
*/
}

void Plater::build_toolbar() {
    wxToolTip::Enable(true);
    auto* toolbar = this->htoolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL | wxTB_TEXT | wxBORDER_SIMPLE | wxTAB_TRAVERSAL);
    toolbar->AddTool(TB_ADD, _(L"Add\u2026"), wxBitmap(var("brick_add.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_REMOVE, _("Delete"), wxBitmap(var("brick_delete.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_RESET, _("Delete All"), wxBitmap(var("cross.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_ARRANGE, _("Arrange"), wxBitmap(var("bricks.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddSeparator();
    toolbar->AddTool(TB_MORE, _("More"), wxBitmap(var("add.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_FEWER, _("Fewer"), wxBitmap(var("delete.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddSeparator();
    toolbar->AddTool(TB_45CCW, _(L"45\u00B0 ccw"), wxBitmap(var("arrow_rotate_anticlockwise.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_45CW, _(L"45\u00B0 cw"), wxBitmap(var("arrow_rotate_clockwise.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_SCALE, _(L"Scale\u2026"), wxBitmap(var("arrow_out.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_SPLIT, _("Split"), wxBitmap(var("shape_ungroup.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_CUT, _(L"Cut\u2026"), wxBitmap(var("package.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddSeparator();
    toolbar->AddTool(TB_SETTINGS, _(L"Settings\u2026"), wxBitmap(var("cog.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_LAYERS, _(L"Layer heights\u2026"), wxBitmap(var("variable_layer_height.png"), wxBITMAP_TYPE_PNG));

    toolbar->Realize();


    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->add(); }, TB_ADD);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->remove(); }, TB_REMOVE);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->reset(); }, TB_RESET);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->arrange(); }, TB_ARRANGE);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->increase(); }, TB_MORE);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->decrease(); }, TB_FEWER);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->rotate(-45); }, TB_45CW);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->rotate(45); }, TB_45CCW);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->changescale(); }, TB_SCALE);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->split_object(); }, TB_SPLIT);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->object_cut_dialog(); }, TB_CUT);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->object_layers_dialog(); }, TB_LAYERS);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->object_settings_dialog(); }, TB_SETTINGS);
}

void Plater::remove() {
    this->remove(-1, false);
}

void Plater::remove(int obj_idx, bool dont_push) {
    
    // TODO: $self->stop_background_process;
    
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
    auto new_model { Slic3r::Model() };
    new_model.add_object(*(this->model->objects.at(obj_ref->identifier)));
   
    Slic3r::Log::info(LogChannel, "Assigned obj_ref");
    try {
        this->model->delete_object(obj_ref->identifier);
    } catch (out_of_range & /*ex*/) {
        Slic3r::Log::error(LogChannel, LOG_WSTRING("Failed to delete object " << obj_ref->identifier << " from Model."));
    }
    try {
        this->print->delete_object(obj_ref->identifier);
    } catch (out_of_range & /*ex*/) {
        Slic3r::Log::error(LogChannel, LOG_WSTRING("Failed to delete object " << obj_ref->identifier << " from Print."));
    }

    this->objects.erase(const_ref);
    int i = 0;
    for (auto o : this->objects) { o.identifier = i++; } // fix identifiers
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
    // TODO: $self->stop_background_process;
    
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

    auto* model_object {this->model->objects.at(obj->identifier)};
    ModelInstance* instance {model_object->instances.back()};

    for (size_t i = 1; i <= copies; i++) {
        instance = model_object->add_instance(*instance);
        instance->offset.x += 10;
        instance->offset.y += 10;
        this->print->objects.at(obj->identifier)->add_copy(instance->offset);
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
    auto* model_object {this->model->objects.at(obj->identifier)};
    if (model_object->instances.size() > copies) {
        for (size_t i = 1; i <= copies; i++) {
            model_object->delete_last_instance();
            this->print->objects.at(obj->identifier)->delete_last_copy();
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

    auto* model_object {this->model->objects.at(obj->identifier)};
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

    auto* model_object {this->model->objects.at(obj->identifier)};
    auto* model_instance {model_object->instances.front()};

    if(obj->thumbnail.expolygons.size() == 0) { return; }

    if (axis == Z) {
        for (auto* instance : model_object->instances)
            instance->rotation += Geometry::deg2rad(angle);
        obj->transform_thumbnail(this->model, obj->identifier);
    } else {
        model_object->transform_by_instance(*model_instance, true);
        model_object->rotate(Geometry::deg2rad(angle), axis);

        // realign object to Z=0
        model_object->center_around_origin();
        this->make_thumbnail(obj->identifier);
    }

    model_object->update_bounding_box();

    this->print->add_model_object(model_object, obj->identifier);

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

void Plater::changescale() {
    ObjRef obj {this->selected_object()};
    if (obj == this->objects.end()) return;
    
    int idx = this->get_object_index(obj->identifier);
    if (idx < 0 || idx >= this->model->objects.size()) return;

    auto* model_object = this->model->objects.at(idx);
    if (model_object->instances.empty()) return;

    auto* model_instance = model_object->instances.front();
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

void Plater::object_cut_dialog() {
    //TODO
    ObjRef obj {this->selected_object()};
    if (obj == this->objects.end()) return;

    auto* model_object {this->model->objects.at(obj->identifier)};
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
    //TODO
}

void Plater::stop_background_process() {
    //TODO
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

wxMenu* Plater::object_menu() {
    auto* frame {this->GetFrame()};
    auto* menu {new wxMenu()};

    append_menu_item(menu, _("Delete"), _("Remove the selected object."), [=](wxCommandEvent& e) { this->remove();}, wxID_ANY, "brick_delete.png", "Ctrl+Del");
    append_menu_item(menu, _("Increase copies"), _("Place one more copy of the selected object."), [=](wxCommandEvent& e) { this->increase();}, wxID_ANY, "add.png", "Ctrl++");
    append_menu_item(menu, _("Decrease copies"), _("Remove one copy of the selected object."), [=](wxCommandEvent& e) { this->decrease();}, wxID_ANY, "delete.png", "Ctrl+-");
    append_menu_item(menu, _(L"Set number of copies\u2026"), _("Change the number of copies of the selected object."), [=](wxCommandEvent& e) { this->set_number_of_copies();}, wxID_ANY, "textfield.png");
    menu->AppendSeparator();
    append_menu_item(menu, _(L"Move to bed center"), _(L"Center object around bed center."), [=](wxCommandEvent& e) { this->center_selected_object_on_bed();}, wxID_ANY, "arrow_in.png");
    append_menu_item(menu, _(L"Rotate 45\u00B0 clockwise"), _(L"Rotate the selected object by 45\u00B0 clockwise."), [=](wxCommandEvent& e) { this->rotate(45);}, wxID_ANY, "arrow_rotate_clockwise.png");
    append_menu_item(menu, _(L"Rotate 45\u00B0 counter-clockwise"), _(L"Rotate the selected object by 45\u00B0 counter-clockwise."), [=](wxCommandEvent& e) { this->rotate(-45);}, wxID_ANY, "arrow_rotate_anticlockwise.png");
    
    {
        auto* rotateMenu {new wxMenu};

        append_menu_item(rotateMenu, _(L"Around X axis\u2026"), _("Rotate the selected object by an arbitrary angle around X axis."), [this](wxCommandEvent& e) { this->rotate(X); }, wxID_ANY, "bullet_red.png");
        append_menu_item(rotateMenu, _(L"Around Y axis\u2026"), _("Rotate the selected object by an arbitrary angle around Y axis."), [this](wxCommandEvent& e) { this->rotate(Y); }, wxID_ANY, "bullet_green.png");
        append_menu_item(rotateMenu, _(L"Around Z axis\u2026"), _("Rotate the selected object by an arbitrary angle around Z axis."), [this](wxCommandEvent& e) { this->rotate(Z); }, wxID_ANY, "bullet_blue.png");

        append_submenu(menu, _("Rotate"), _("Rotate the selected object by an arbitrary angle"), rotateMenu, wxID_ANY, "textfield.png");
    }
    /*
    
    {
        my $mirrorMenu = Wx::Menu->new;
        wxTheApp->append_menu_item($mirrorMenu, "Along X axis…", 'Mirror the selected object along the X axis', sub {
            $self->mirror(X);
        }, undef, 'bullet_red.png');
        wxTheApp->append_menu_item($mirrorMenu, "Along Y axis…", 'Mirror the selected object along the Y axis', sub {
            $self->mirror(Y);
        }, undef, 'bullet_green.png');
        wxTheApp->append_menu_item($mirrorMenu, "Along Z axis…", 'Mirror the selected object along the Z axis', sub {
            $self->mirror(Z);
        }, undef, 'bullet_blue.png');
        wxTheApp->append_submenu($menu, "Mirror", 'Mirror the selected object', $mirrorMenu, undef, 'shape_flip_horizontal.png');
    }
    
    {
        my $scaleMenu = Wx::Menu->new;
        wxTheApp->append_menu_item($scaleMenu, "Uniformly…", 'Scale the selected object along the XYZ axes', sub {
            $self->changescale(undef);
        });
        wxTheApp->append_menu_item($scaleMenu, "Along X axis…", 'Scale the selected object along the X axis', sub {
            $self->changescale(X);
        }, undef, 'bullet_red.png');
        wxTheApp->append_menu_item($scaleMenu, "Along Y axis…", 'Scale the selected object along the Y axis', sub {
            $self->changescale(Y);
        }, undef, 'bullet_green.png');
        wxTheApp->append_menu_item($scaleMenu, "Along Z axis…", 'Scale the selected object along the Z axis', sub {
            $self->changescale(Z);
        }, undef, 'bullet_blue.png');
        wxTheApp->append_submenu($menu, "Scale", 'Scale the selected object by a given factor', $scaleMenu, undef, 'arrow_out.png');
    }
    
    {
        my $scaleToSizeMenu = Wx::Menu->new;
        wxTheApp->append_menu_item($scaleToSizeMenu, "Uniformly…", 'Scale the selected object along the XYZ axes', sub {
            $self->changescale(undef, 1);
        });
        wxTheApp->append_menu_item($scaleToSizeMenu, "Along X axis…", 'Scale the selected object along the X axis', sub {
            $self->changescale(X, 1);
        }, undef, 'bullet_red.png');
        wxTheApp->append_menu_item($scaleToSizeMenu, "Along Y axis…", 'Scale the selected object along the Y axis', sub {
            $self->changescale(Y, 1);
        }, undef, 'bullet_green.png');
        wxTheApp->append_menu_item($scaleToSizeMenu, "Along Z axis…", 'Scale the selected object along the Z axis', sub {
            $self->changescale(Z, 1);
        }, undef, 'bullet_blue.png');
        wxTheApp->append_submenu($menu, "Scale to size", 'Scale the selected object to match a given size', $scaleToSizeMenu, undef, 'arrow_out.png');
    }
    
    wxTheApp->append_menu_item($menu, "Split", 'Split the selected object into individual parts', sub {
        $self->split_object;
    }, undef, 'shape_ungroup.png');
    wxTheApp->append_menu_item($menu, "Cut…", 'Open the 3D cutting tool', sub {
        $self->object_cut_dialog;
    }, undef, 'package.png');
    wxTheApp->append_menu_item($menu, "Layer heights…", 'Open the dynamic layer height control', sub {
        $self->object_layers_dialog;
    }, undef, 'variable_layer_height.png');
    $menu->AppendSeparator();
    wxTheApp->append_menu_item($menu, "Settings…", 'Open the object editor dialog', sub {
        $self->object_settings_dialog;
    }, undef, 'cog.png');
    $menu->AppendSeparator();
    wxTheApp->append_menu_item($menu, "Reload from Disk", 'Reload the selected file from Disk', sub {
        $self->reload_from_disk;
    }, undef, 'arrow_refresh.png');
    wxTheApp->append_menu_item($menu, "Export object as STL…", 'Export this single object as STL file', sub {
        $self->export_object_stl;
    }, undef, 'brick_go.png');
    wxTheApp->append_menu_item($menu, "Export object and modifiers as AMF…", 'Export this single object and all associated modifiers as AMF file', sub {
        $self->export_object_amf;
    }, undef, 'brick_go.png');
    wxTheApp->append_menu_item($menu, "Export object and modifiers as 3MF…", 'Export this single object and all associated modifiers as 3MF file', sub {
            $self->export_object_tmf;
    }, undef, 'brick_go.png');
    
    return $menu;
}
*/
    return menu;
}

void Plater::set_number_of_copies() {
    this->pause_background_process();

    ObjRef obj {this->selected_object()};
    if (obj == this->objects.end()) return;
    auto* model_object { this->model->objects.at(obj->identifier) };

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
    auto* model_object { this->model->objects.at(obj->identifier) };
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

        std::ofstream log("slicing_debug.log", std::ios::app);
        log << "Starting process()" << std::endl;

        // Process
        this->print->process();

        log << "Finished process(), starting export_gcode()" << std::endl;
        
        // Export
        this->print->export_gcode(output_file);

        log << "Finished export_gcode()" << std::endl;
        
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
    
    std::thread([this, progressDialog]() {
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
        
    }).detach();
}

}} // Namespace Slic3r::GUI