#ifndef PLATER_HPP
#define PLATER_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/notebook.h>
#include <wx/toolbar.h>
#include <wx/menu.h>
#include <wx/simplebook.h>

#include <stack>

#include "libslic3r.h"
#include "Model.hpp"
#include "Print.hpp"
#include "Config.hpp"
#include "misc_ui.hpp"

#include "Preset.hpp"

#include "Plater/PlaterObject.hpp"

#include "Plater/Plate3D.hpp"
#include "Plater/Preview3D.hpp"
// #include "Plater/PreviewDLP.hpp"
#include "Plater/PresetChooser.hpp"

#include "Settings.hpp"
#include "Theme/ThemeManager.hpp"
#include "MainFrame.hpp"

namespace Slic3r { namespace GUI {

class PresetChooser; // forward dec
using UndoOperation = int;

enum class UndoCmd {
    Remove, Add, Reset, Increase, Decrease, Rotate
};


enum class Zoom {
    In, Out
};

using ObjIdx = unsigned int;
using ObjRef = std::vector<PlaterObject>::iterator;

class PlaterObject;

class MainFrame;

/// Struct to group object info text fields together
struct info_fields {
    wxChoice* choice {nullptr};
    wxStaticText* copies {nullptr};
    wxStaticText* size {nullptr};
    wxStaticText* volume {nullptr};
    wxStaticText* facets {nullptr};
    wxStaticText* materials {nullptr};
    wxStaticText* manifold {nullptr};
    wxStaticBitmap* manifold_warning_icon {nullptr};
};

/// Extension of wxPanel class to handle the main plater.
/// 2D, 3D, preview, etc tabs.
class Plater : public wxPanel 
{
public:
    Plater(wxWindow* parent, const wxString& title);

    /// User-level function called through external interface.
    /// Pops file dialog.
    void add();

    void load_files(const std::vector<std::string>& files);

    /// Remove a selected model from the plater.
    void remove(int obj_idx, bool dont_push = false);
    void remove();
    
    /// Arrange models via a simple packing mechanism based on bounding boxes.
    void arrange();

    /// Ask if there are any unsaved changes.
    bool prompt_unsaved_changes() { return true; }

    void add_undo_operation(UndoCmd cmd, int obj_id, Slic3r::Model& model);

    /// Push an undo op onto the stack.
    void add_undo_operation(UndoCmd cmd, std::vector<int>& obj_ids, Slic3r::Model& model);
    
    /// Undo for increase/decrease 
    void add_undo_operation(UndoCmd cmd, int obj_id, size_t copies);

    /// Undo for increase/decrease 
    void add_undo_operation(UndoCmd cmd, int obj_id, double angle, Axis axis);

    /// Create menu for object.
    wxMenu* object_menu();


    /// Retrieve the identifier for the currently selected preset.
    void undo() {};
    void redo() {};

    void select_next() {};
    void select_prev() {};
    void zoom(Zoom dir) {};

    void export_gcode();
    void export_amf() {};
    void export_tmf() {};
    void export_stl() {};

    void slice();


    void show_preset_editor(preset_t preset, unsigned int idx);

    void select_view_3d();
    void select_view_preview();
    void select_view(Direction dir);
private:
    std::shared_ptr<Slic3r::Print> print {std::make_shared<Print>(Slic3r::Print())};
    std::shared_ptr<Slic3r::Model> model {std::make_shared<Model>(Slic3r::Model())};

    std::shared_ptr<Slic3r::Config> config { Slic3r::Config::new_from_defaults(
        {"bed_shape", "complete_objects", "extruder_clearance_radius", "skirts", "skirt_distance", 
        "brim_width", "serial_port", "serial_speed", "host_type", "print_host", "octoprint_apikey",
        "shortcuts", "filament_colour", "duplicate_distance"})};

    bool processed {false};

    std::vector<PlaterObject> objects {}; //< Main object vector.

    size_t object_identifier {0U}; //< Counter for adding objects to Slic3r. Increment after adding each object.

    std::stack<UndoOperation> _undo {}; 
    std::stack<UndoOperation> _redo {}; 

    wxSimplebook* preview_notebook {nullptr};
    wxBoxSizer* right_sizer {new wxBoxSizer(wxVERTICAL)};

    // Replacement for native toolbar
    wxPanel* toolbar_panel {nullptr}; 
    std::map<int, wxWindow*> toolbar_tools; 

    // wxToolBar* htoolbar {nullptr}; //< toolbar for non-MSW platforms.
    wxBoxSizer* btoolbar {nullptr}; //< button-based toolbar for Windows


    Plate3D* canvas3D {nullptr}; //< 3D plater canvas

    Preview3D* preview3D {nullptr}; //< 3D Preview 

    // PreviewDLP* previewDLP {nullptr}; //< DLP/SLA Preview canvas

    wxStaticBoxSizer* object_info_size {nullptr};

    /// Handles the actual load of the file from the dialog handoff.
    std::vector<int> load_file(const std::string file, const int obj_idx_to_load = -1);

    const std::string LogChannel {"GUI_Plater"}; //< Which log these messages should go to.

    /// Populate the PlaterObject vector.
    std::vector<int> load_model_objects(ModelObject* model_object);
    std::vector<int> load_model_objects(ModelObjectPtrs model_objects);

    bool scaled_down {false};
    bool outside_bounds {false};

    /// Method to get the top-level window and cast it as a MainFrame.
    MainFrame* GetFrame();

    void select_object(ObjRef obj_idx);
    void select_object(ObjIdx obj_idx);

    /// Overload to unselect objects
    void select_object();
    
    int get_object_index(ObjIdx object_id);

    /// Get the center of the configured bed's bounding box.
    Slic3r::Pointf bed_centerf() {
        const auto& bed_shape { Slic3r::Polygon::new_scale(this->config->get<ConfigOptionPoints>("bed_shape").values) };
        const auto& bed_center {BoundingBox(bed_shape).center()};
        return Slic3r::Pointf::new_unscale(bed_center);
    }

    /// Build thumbnails for the models
    void make_thumbnail(size_t idx);

    /// Complete thumbnail transformation and refresh canvases  
    void on_thumbnail_made(size_t idx); 

    /// Issue a repaint event to all of the canvasses.
    void refresh_canvases();

    /// Action to take when selection changes. Update platers, etc.
    void selection_changed();

    /// Run everything that needs to happen when models change.
    /// Includes updating canvases, reloading menus, etc.
    void on_model_change(bool force_autocenter = false);

    /// Searches the object vector for the first selected object and returns an iterator to it.
    ObjRef selected_object();

    /// Create and launch dialog for object settings.
    void object_settings_dialog();
    void object_settings_dialog(ObjIdx obj_idx);
    void object_settings_dialog(ObjRef obj);

    /// Instantiate the toolbar 
    void build_toolbar();

    /// Clear plate.
    void reset(bool dont_push = false);

    /// Make instances of the currently selected model.
    void increase(size_t copies = 1, bool dont_push = false); 

    /// Remove instances of the currently selected model.
    void decrease(size_t copies = 1, bool dont_push = false); 

    /// Rotate the currently selected model, triggering a user prompt.
    void rotate(Axis axis = Z, bool dont_push = false); 
    /// Rotate the currently selected model.
    void rotate(double angle, Axis axis = Z, bool dont_push = false); 

    /// Mirror the currently selected model.
    void mirror(Axis axis, bool dont_push = false);

    /// Reload the selected object from disk.
    void reload_from_disk();

    /// Export the single selected object.
    void export_object_stl();
    void export_object_amf();
    void export_object_tmf();

    /// Separate a multipart model to its component interfaces.
    void split_object(); 

    /// Prompt a change of scaling.
    void changescale(bool to_size = false);
    void changescale(Axis axis, bool to_size = false);

    /// Open the dialog to perform a cut on the current model.
    void object_cut_dialog();

    /// Open a menu to configure the layer heights.
    void object_layers_dialog();

    /// Process a change in the object list.
    void object_list_changed();

    /// Halt ongoing background processes.
    void stop_background_process();

    void start_background_process();

    void pause_background_process();
    void resume_background_process();

    /// Move the selected object to the center of bed.
    void center_selected_object_on_bed();

    void set_number_of_copies();

    /// Struct containing various object info fields.
    info_fields object_info;

    PresetChooser* _presets;

    void load_presets();

};


template <typename T>
static void add_info_field(wxWindow* parent, T*& field, wxString name, wxGridSizer* sizer) {
    name << ":";
    auto* text {new wxStaticText(parent, wxID_ANY, name, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT)};
    text->SetFont(ui_settings->small_font());
    if (ThemeManager::IsDark()) text->SetForegroundColour(*wxWHITE);
    sizer->Add(text, 0);

    field = new wxStaticText(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
    field->SetFont(ui_settings->small_font());
    if (ThemeManager::IsDark()) field->SetForegroundColour(*wxWHITE);
    sizer->Add(field, 0);
}

} } // Namespace Slic3r::GUI

#endif  // PLATER_HPP
