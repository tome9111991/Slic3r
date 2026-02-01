#ifndef slic3r_ObjectSettingsDialog_hpp_
#define slic3r_ObjectSettingsDialog_hpp_

#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/notebook.h>

#include "libslic3r/Model.hpp"

namespace Slic3r {
namespace GUI {

class ObjectSettingsDialog : public wxDialog {
public:
    ObjectSettingsDialog(wxWindow* parent, ModelObject* model_object);
    ~ObjectSettingsDialog() {}

private:
    ModelObject* m_model_object;
    wxNotebook* m_tabpanel;
    wxGrid* m_layers_grid;

    void build_layers_tab(wxWindow* parent);
    void load_layers();
    bool save_layers();
    
    // Helper to get ranges from grid
    t_layer_height_ranges get_ranges_from_grid();

    void on_ok(wxCommandEvent& event);
};

} // namespace GUI
} // namespace Slic3r

#endif
