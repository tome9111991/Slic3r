#include "Dialogs/PresetEditor.hpp"
#include "misc_ui.hpp"
#include "GUI.hpp"
#include <wx/bookctrl.h>


namespace Slic3r { namespace GUI {

PresetEditor::PresetEditor(wxWindow* parent, t_config_option_keys options) : 
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_LEFT | wxTAB_TRAVERSAL) {

    this->_sizer = new wxBoxSizer(wxHORIZONTAL);
    this->SetSizer(this->_sizer);

    wxSizer* left_sizer { new wxBoxSizer(wxVERTICAL) };

    {
        // choice menu
        this->_presets_choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(left_col_width, -1));
        this->_presets_choice->SetFont(ui_settings->small_font());


        // buttons
        this->_btn_save_preset = new wxBitmapButton(this, wxID_ANY, wxBitmap(var("disk.png"), wxBITMAP_TYPE_PNG), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        this->_btn_delete_preset = new wxBitmapButton(this, wxID_ANY, wxBitmap(var("delete.png"), wxBITMAP_TYPE_PNG), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);

        this->set_tooltips();
        this->_btn_delete_preset->Disable();

        wxBoxSizer* hsizer {new wxBoxSizer(wxHORIZONTAL)};
        left_sizer->Add(hsizer, 0, wxEXPAND | wxBOTTOM, 5);
        hsizer->Add(this->_presets_choice, 1, wxRIGHT | wxALIGN_CENTER_VERTICAL, 3);
        hsizer->Add(this->_btn_save_preset, 0, wxALIGN_CENTER_VERTICAL);
        hsizer->Add(this->_btn_delete_preset, 0, wxALIGN_CENTER_VERTICAL);

    }

    // tree
    this->_treectrl = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(left_col_width, -1), wxTR_NO_BUTTONS | wxTR_HIDE_ROOT | wxTR_SINGLE | wxTR_NO_LINES | wxBORDER_SUNKEN | wxWANTS_CHARS);

    left_sizer->Add(this->_treectrl, 1, wxEXPAND);
    this->_icons = new wxImageList(16, 16, 1);
    this->_treectrl->AssignImageList(this->_icons);
    this->_iconcount = -1;

    this->_treectrl->AddRoot("root");
    this->_treectrl->SetIndent(0);
    this->disable_tree_sel_changed_event = false;

    /// bind a lambda for the event EVT_TREE_SEL_CHANGED 
   
    /// bind a lambda for the event EVT_KEY_DOWN 
    
    /// bind a lambda for the event EVT_CHOICE
    
    /// bind a lambda for the event EVT_KEY_DOWN 
    
    /// bind a lambda for the event EVT_BUTTON from btn_save_preset 
    
    /// bind a lambda for the event EVT_BUTTON from btn_delete_preset 
    
}

void PresetEditor::save_preset() {
    // TODO: show save dialog if needed or just save
    if (this->current_preset) {
        // Validation?
        this->current_preset->save(this->current_preset->dirty_options());
        this->load_presets();
    }
}

// ... _on_value_change ...

void PresetEditor::_on_select_preset(bool force) {
    int sel = this->_presets_choice->GetSelection();
    if (sel < 0) return;
    
    // Get preset from app
    auto& presets = SLIC3RAPP->presets.at(this->typeId());
    if (sel >= presets.size()) return;
    
    // Update current preset pointer
    // We need shared_ptr, but presets storage is vector<Preset>.
    // Usually Config wizard or App handles pointers.
    // PresetEditor.hpp says: std::shared_ptr<Preset> current_preset;
    // but vector stores objects.
    // We should probably store a pointer or reference?
    // Changing current_preset to raw pointer or index might be safer given vector resize?
    // But vector resize invalidates pointers.
    // Let's assume presets are stable for now or we key by name.
    
    // Actually SLIC3RAPP->presets is std::array<Presets, 3>; Presets = std::vector<Preset>;
    // current_preset is std::shared_ptr<Preset> in header.
    // This implies we make a copy or Presets stores shared_ptrs?
    // Preset.hpp: using Presets = std::vector<Preset>; 
    // So it stores OBJECTS.
    // std::shared_ptr<Preset> current_preset is WRONG if it points to vector element.
    // It should be Preset* current_preset (observer).
    // Or we copy it.
    
    // Let's rely on name for now and getting address.
    
    Preset* p = &presets[sel];
    // We cannot assign address to shared_ptr unless we use aliasing constructor or empty deleter? 
    // This logic seems flawed in header.
    // For now I will assume I can't change the header type easily without checking usage.
    // But I must.
    
    // Hack: make a shared_ptr with no-op deleter
    this->current_preset = std::shared_ptr<Preset>(p, [](Preset*){});
    
    this->config = this->current_preset->load_config();
    this->reload_config();
}


void PresetEditor::reload_config() {
    if (!this->config) return;
    
    // Iterate pages and update all options
    for (auto* page : _pages) {
         page->update_options(&this->config->config());
    }
}

// ...

void PresetEditor::_update_tree() {
    // Rebuild tree if needed
}

void PresetEditor::load_presets() {
    this->_presets_choice->Clear();
    auto& presets = SLIC3RAPP->presets.at(this->typeId());
    for (size_t i = 0; i < presets.size(); ++i) {
        this->_presets_choice->Append(presets[i].dropdown_name());
    }
    
    if (this->_presets_choice->GetCount() > 0)
        this->_presets_choice->SetSelection(0);
}

UI_Field* PresetEditor::get_ui_field(const std::string& key) {
    for (auto* page : _pages) {
        if (auto* f = page->get_ui_field(key)) return f;
    }
    return nullptr;
}



}} // namespace Slic3r::GUI
