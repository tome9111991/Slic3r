#include "Dialogs/PresetEditor.hpp"
#include "misc_ui.hpp"
#include "GUI.hpp"
#include <wx/bookctrl.h>
#include "libslic3r/PrintConfig.hpp"
#include "Log.hpp"

namespace Slic3r { namespace GUI {

PresetEditor::PresetEditor(wxWindow* parent, t_config_option_keys options) : 
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_LEFT | wxTAB_TRAVERSAL) {

    this->_sizer = new wxBoxSizer(wxHORIZONTAL);
    this->SetSizer(this->_sizer);

    wxSizer* left_sizer = new wxBoxSizer(wxVERTICAL);

    {
        // choice menu
        this->_presets_choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(left_col_width, -1));
        this->_presets_choice->SetFont(ui_settings->small_font());

        // buttons
        this->_btn_save_preset = new wxBitmapButton(this, wxID_ANY, wxBitmap(var("disk.png"), wxBITMAP_TYPE_PNG), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        this->_btn_delete_preset = new wxBitmapButton(this, wxID_ANY, wxBitmap(var("delete.png"), wxBITMAP_TYPE_PNG), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);

        this->_btn_delete_preset->Disable();

        wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
        left_sizer->Add(hsizer, 0, wxEXPAND | wxBOTTOM, 5);
        hsizer->Add(this->_presets_choice, 1, wxRIGHT | wxALIGN_CENTER_VERTICAL, 3);
        hsizer->Add(this->_btn_save_preset, 0, wxALIGN_CENTER_VERTICAL);
        hsizer->Add(this->_btn_delete_preset, 0, wxALIGN_CENTER_VERTICAL);

        this->_presets_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
            this->_on_select_preset();
        });

        this->_btn_save_preset->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            this->save_preset();
        });
    }

    // tree
    this->_treectrl = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(left_col_width, -1), wxTR_NO_BUTTONS | wxTR_HIDE_ROOT | wxTR_SINGLE | wxTR_NO_LINES | wxBORDER_SUNKEN | wxWANTS_CHARS);

    left_sizer->Add(this->_treectrl, 1, wxEXPAND);
    
    this->_sizer->Add(left_sizer, 0, wxEXPAND | wxALL, 5);

    // Right side content area
    
    this->_icons = new wxImageList(16, 16, 1);
    this->_treectrl->AssignImageList(this->_icons);
    this->_iconcount = -1;

    this->_treectrl->AddRoot("root");
    this->_treectrl->SetIndent(0);
}

PresetPage* PresetEditor::add_options_page(const wxString& _title, const wxString& _icon) {
    auto* page = new PresetPage(this, _title); 
    page->Hide(); // Hidden by default
    this->_sizer->Add(page, 1, wxEXPAND | wxALL, 5);
    _pages.push_back(page);
    
    // Add to tree
    this->_treectrl->AppendItem(this->_treectrl->GetRootItem(), _title);
    
    // Bind Callback
    page->on_change = [this](const std::string& key, boost::any value) {
        if (!this->config) return;
        
        // Very basic type mapping
        try {
            if (value.type() == typeid(bool)) 
                this->config->set(key, boost::any_cast<bool>(value));
            else if (value.type() == typeid(int)) 
                this->config->set(key, boost::any_cast<int>(value));
            else if (value.type() == typeid(double)) 
                this->config->set(key, boost::any_cast<double>(value));
            else if (value.type() == typeid(std::string)) 
                this->config->set(key, boost::any_cast<std::string>(value));
            
             Slic3r::Log::info(this->LogChannel(), "Updated config: " + key);
             
             // Visual feedback for dirty state
             if (this->current_preset && this->current_preset->dirty()) {
                 int sel = this->_presets_choice->GetSelection();
                 if (sel != wxNOT_FOUND) {
                     wxString label = this->_presets_choice->GetString(sel);
                     if (!label.EndsWith(" *")) {
                         this->_presets_choice->SetString(sel, label + " *");
                     }
                 }
             }
        } catch(std::exception& e) {
             Slic3r::Log::error(this->LogChannel(), "Failed to update config for " + key + ": " + e.what());
        }
    };
    
    return page;
}

void PresetEditor::save_preset() {
    if (this->current_preset) {
        this->current_preset->save();
        // Reload presets list to reflect changes (if any)
        // this->load_presets(); 
    }
}

void PresetEditor::_on_select_preset(bool force) {
    int sel = this->_presets_choice->GetSelection();
    if (sel < 0) return;
    
    auto& presets = SLIC3RAPP->presets.at(this->typeId());
    if (sel >= presets.size()) return;
    
    Preset* p = &presets[sel];
    // Hack for shared_ptr
    this->current_preset = std::shared_ptr<Preset>(p, [](Preset*){});
    
    this->config = this->current_preset->load_config();
    this->reload_config();
    
    // Show first page
    if (!_pages.empty()) {
        for(auto* p : _pages) p->Hide();
        _pages[0]->Show();
        this->Layout();
    }
}

void PresetEditor::reload_config() {
    if (!this->config) return;
    for (auto* page : _pages) {
         page->update_options(&this->config->config());
    }
}

UI_Field* PresetEditor::get_ui_field(const std::string& key) {
    for (auto* page : _pages) {
         UI_Field* f = page->get_ui_field(key);
         if (f) return f;
    }
    return nullptr;
}

void PresetEditor::_update(const std::string& opt_key) {}
void PresetEditor::_build() {}
void PresetEditor::_on_preset_loaded() {}

void PresetEditor::_update_tree() {}

void PresetEditor::load_presets() {
    this->_presets_choice->Clear();
    try {
        if (!SLIC3RAPP) return;
        auto& presets = SLIC3RAPP->presets.at(this->typeId());
        for (size_t i = 0; i < presets.size(); ++i) {
            this->_presets_choice->Append(presets[i].dropdown_name());
        }
        if (this->_presets_choice->GetCount() > 0) {
            this->_presets_choice->SetSelection(0);
            _on_select_preset();
        }
    } catch (...) {
        // App might not be fully init
    }
}

void PresetEditor::_on_value_change(std::string opt_key) {}

void PresetEditor::reload_compatible_printers_widget() {}
wxSizer* PresetEditor::compatible_printers_widget() { return nullptr; }

}} // namespace Slic3r::GUI
