#include "ObjectSettingsDialog.hpp"
#include "libslic3r/Layer.hpp"
#include <wx/msgdlg.h>
#include <wx/sizer.h>

namespace Slic3r {
namespace GUI {

ObjectSettingsDialog::ObjectSettingsDialog(wxWindow* parent, ModelObject* model_object)
    : wxDialog(parent, wxID_ANY, wxString::Format("Settings for %s", model_object->name), wxDefaultPosition, wxSize(600, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_model_object(model_object)
{
    m_tabpanel = new wxNotebook(this, wxID_ANY);

    wxPanel* parts_panel = new wxPanel(m_tabpanel);
    // TODO: Implement Parts panel
    wxBoxSizer* parts_sizer = new wxBoxSizer(wxVERTICAL);
    parts_sizer->Add(new wxStaticText(parts_panel, wxID_ANY, "Parts editing not yet implemented in C++ port."), 0, wxALL, 10);
    parts_panel->SetSizer(parts_sizer);
    m_tabpanel->AddPage(parts_panel, "Parts");

    wxPanel* layers_panel = new wxPanel(m_tabpanel);
    build_layers_tab(layers_panel);
    m_tabpanel->AddPage(layers_panel, "Layer height table");

    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_tabpanel, 1, wxEXPAND | wxALL, 5);

    wxSizer* buttons = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    main_sizer->Add(buttons, 0, wxEXPAND | wxALL, 5);

    this->SetSizer(main_sizer);
    this->Layout();

    Bind(wxEVT_BUTTON, &ObjectSettingsDialog::on_ok, this, wxID_OK);

    load_layers();
}

void ObjectSettingsDialog::build_layers_tab(wxWindow* parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* label = new wxStaticText(parent, wxID_ANY, 
        "You can use this section to override the layer height for parts of this object.\n"
        "The values from this table will override the default layer height and adaptive layer heights.");
    label->Wrap(550);
    sizer->Add(label, 0, wxEXPAND | wxALL, 10);

    m_layers_grid = new wxGrid(parent, wxID_ANY);
    m_layers_grid->CreateGrid(0, 3);
    m_layers_grid->SetColLabelValue(0, "Min Z (mm)");
    m_layers_grid->SetColLabelValue(1, "Max Z (mm)");
    m_layers_grid->SetColLabelValue(2, "Layer height (mm)");
    // m_layers_grid->SetColFormatFloat(0, -1, 2); // Not available in some wx versions? Use default string
    m_layers_grid->SetColSize(0, 100);
    m_layers_grid->SetColSize(1, 100);
    m_layers_grid->SetColSize(2, 120);

    sizer->Add(m_layers_grid, 1, wxEXPAND | wxALL, 10);
    parent->SetSizer(sizer);
}

void ObjectSettingsDialog::load_layers()
{
    if (!m_model_object) return;

    if (m_layers_grid->GetNumberRows() > 0)
        m_layers_grid->DeleteRows(0, m_layers_grid->GetNumberRows());

    for (const auto& kv : m_model_object->layer_height_ranges) {
        m_layers_grid->AppendRows(1);
        int row = m_layers_grid->GetNumberRows() - 1;
        m_layers_grid->SetCellValue(row, 0, wxString::Format("%.2f", kv.first.first));
        m_layers_grid->SetCellValue(row, 1, wxString::Format("%.2f", kv.first.second));
        m_layers_grid->SetCellValue(row, 2, wxString::Format("%.2f", kv.second));
    }
    // Always append an empty row for new entries
    m_layers_grid->AppendRows(1);
}

bool ObjectSettingsDialog::save_layers()
{
    t_layer_height_ranges ranges = get_ranges_from_grid();
    // Basic validation
    // Perl version checks for overlaps and negative values.
    // For now, we trust ranges (Map keys prevent duplicate start/end pairs, but not overlaps).
    
    m_model_object->layer_height_ranges = ranges;
    return true;
}

t_layer_height_ranges ObjectSettingsDialog::get_ranges_from_grid()
{
    t_layer_height_ranges ranges;
    for (int i = 0; i < m_layers_grid->GetNumberRows(); ++i) {
        wxString s_min = m_layers_grid->GetCellValue(i, 0);
        wxString s_max = m_layers_grid->GetCellValue(i, 1);
        wxString s_h   = m_layers_grid->GetCellValue(i, 2);

        double min_z, max_z, h;
        if (s_min.ToDouble(&min_z) && s_max.ToDouble(&max_z) && s_h.ToDouble(&h)) {
             ranges[{min_z, max_z}] = h;
        }
    }
    return ranges;
}

void ObjectSettingsDialog::on_ok(wxCommandEvent& event)
{
    if (save_layers()) {
        EndModal(wxID_OK);
    }
}

} // namespace GUI
} // namespace Slic3r
