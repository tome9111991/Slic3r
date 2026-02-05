#include "Dialogs/PresetEditor.hpp"
#include "misc_ui.hpp"
#include "GUI.hpp"
#include <wx/bookctrl.h>
#include "libslic3r/PrintConfig.hpp"
#include "Log.hpp"
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
#include "../Theme/ThemeManager.hpp"

namespace Slic3r { namespace GUI {

class PageData : public wxTreeItemData {
public:
    PresetPage* page;
    PageData(PresetPage* p) : page(p) {}
};

PresetEditor::PresetEditor(wxWindow* parent, t_config_option_keys options) : 
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_LEFT | wxTAB_TRAVERSAL) {

    this->_sizer = new wxBoxSizer(wxHORIZONTAL);
    this->SetSizer(this->_sizer);

    wxSizer* left_sizer = new wxBoxSizer(wxVERTICAL);

    {
        // choice menu
        // choice menu
        this->_presets_choice = new ThemedSelect(this, wxID_ANY, wxArrayString(), wxDefaultPosition, wxSize(left_col_width, 30));

        // buttons
        this->_btn_save_preset = new ThemedButton(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
        this->_btn_save_preset->SetBitmap(get_bmp_bundle("disk.svg"));
        
        this->_btn_delete_preset = new ThemedButton(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
        this->_btn_delete_preset->SetBitmap(get_bmp_bundle("delete.svg"));

        this->_btn_delete_preset->Enable(false);

        wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
        left_sizer->Add(hsizer, 0, wxEXPAND | wxBOTTOM, 5);
        hsizer->Add(this->_presets_choice, 1, wxRIGHT | wxALIGN_CENTER_VERTICAL, 3);
        hsizer->Add(this->_btn_save_preset, 0, wxALIGN_CENTER_VERTICAL);
        hsizer->Add(this->_btn_delete_preset, 0, wxALIGN_CENTER_VERTICAL);

        this->_presets_choice->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent&) {
            this->_on_select_preset();
        });

        this->_btn_save_preset->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            this->save_preset();
        });
        
        this->_btn_delete_preset->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            if (this->current_preset && !this->current_preset->default_preset) {
                if (wxMessageBox(_("Are you sure you want to delete this preset?"), _("Confirm Deletion"), wxYES_NO | wxICON_QUESTION) == wxYES) {
                    this->current_preset->delete_preset();
                    this->load_presets();
                }
            }
        });
    }

    // tree
    this->_treectrl = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(left_col_width, -1), wxTR_NO_BUTTONS | wxTR_HIDE_ROOT | wxTR_SINGLE | wxTR_NO_LINES | wxBORDER_SUNKEN | wxWANTS_CHARS);
    
    this->_treectrl->Bind(wxEVT_TREE_SEL_CHANGED, [this](wxTreeEvent& event) {
        wxTreeItemId item = event.GetItem();
        if (item.IsOk()) {
             auto* data = dynamic_cast<PageData*>(this->_treectrl->GetItemData(item));
             if (data && data->page) {
                 for(auto* p : _pages) p->Hide();
                 data->page->Show();
                 this->Layout();
             }
        }
    });

    left_sizer->Add(this->_treectrl, 1, wxEXPAND);
    
    this->_sizer->Add(left_sizer, 0, wxEXPAND | wxALL, 5);

    // Right side content area
    
    this->_icons = new wxImageList(16, 16, 1);
    this->_treectrl->AssignImageList(this->_icons);
    this->_iconcount = -1;

    this->_treectrl->AddRoot("root");
    this->_treectrl->SetIndent(0);
    
    // Ensure initial coloring is correct
    // We must invoke this because the panel is created with default colors initially
    // and if UpdateUI() hasn't run recently on this specific instance, it stays default.
    ThemeManager::ApplyThemeRecursive(this);
}

PresetPage* PresetEditor::add_options_page(const wxString& _title, const wxString& _icon) {
    auto* page = new PresetPage(this, _title); 
    page->Hide(); // Hidden by default
    this->_sizer->Add(page, 1, wxEXPAND | wxALL, 5);
    _pages.push_back(page);
    
    int icon_idx = -1;
    if (!_icon.empty()) {
        wxBitmap bmp = get_bmp_bundle(_icon).GetBitmap(wxSize(16, 16));
        if (bmp.IsOk()) {
             icon_idx = this->_icons->Add(bmp);
        }
    }
    
    // Add to tree
    this->_treectrl->AppendItem(this->_treectrl->GetRootItem(), _title, icon_idx, icon_idx, new PageData(page));
    
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
             if (this->current_preset) {
                 // Check if the overall preset is dirty (for the * in the name)
                 if (this->current_preset->dirty()) {
                     int sel = this->_presets_choice->GetSelection();
                     if (sel != wxNOT_FOUND) {
                         wxString label = this->_presets_choice->GetString(sel);
                         if (!label.EndsWith(" *")) {
                             this->_presets_choice->SetString(sel, label + " *");
                         }
                     }
                 }
                 
                 // Check if THIS specific option is dirty
                 // Manual check:
                 bool is_dirty = false;
                 
                 // Get safe pointers to the configs
                 auto pristine_cfg = this->current_preset->config().lock();
                 auto current_cfg = this->config; // The one we are editing right now

                 if (pristine_cfg && current_cfg && pristine_cfg->has(key)) {
                      auto opt_pristine = pristine_cfg->config().option(key);
                      auto opt_current = current_cfg->config().option(key);
                      
                      if (opt_pristine && opt_current) {
                          auto val_pristine = opt_pristine->serialize();
                          auto val_current = opt_current->serialize();
                          is_dirty = (val_pristine != val_current);
                          Slic3r::Log::info(this->LogChannel(), "Dirty Check for " + key + ": IsDirty=" + (is_dirty?"Yes":"No") + " Pristine='" + val_pristine + "' Current='" + val_current + "'");
                      }
                 } else {
                      is_dirty = true;
                      Slic3r::Log::info(this->LogChannel(), "Dirty Check for " + key + ": IsDirty=Yes (Missing in pristine or invalid pointers)");
                 }
                 
                 UI_Field* field = this->get_ui_field(key);
                 if (field) {
                     field->set_dirty_status(is_dirty);
                     Slic3r::Log::info(this->LogChannel(), "Set visual status for " + key);
                 } else {
                     Slic3r::Log::warn(this->LogChannel(), "Field not found for " + key);
                 }
             }
        } catch(std::exception& e) {
             Slic3r::Log::error(this->LogChannel(), "Failed to update config for " + key + ": " + e.what());
        }
    };
    
    page->on_quick_setting_change = [this](const std::string& key, bool active) {
        Slic3r::Log::info(this->LogChannel(), "Quick Setting Toggle: " + key);
        
        // Use global settings to persist
        ui_settings->toggle_quick_setting(key);
        bool now_active = ui_settings->is_quick_setting(key);

        // Propagate up (e.g. to Plater to refresh the quick settings panel)
        if (this->on_quick_setting_change) {
            this->on_quick_setting_change(key, now_active);
        }
        
        // Visual Feedback: Toggle the icon immediately for all pages
        for (auto* p : _pages) {
            p->set_quick_setting_status(key, now_active);
        }
    };

    return page;
}

void PresetEditor::save_preset() {
    // 1. Commit pending changes manually.
    //    We simulate KILL_FOCUS and TEXT_ENTER events on the active widget to force Slic3r
    //    to update the configuration from the UI value immediately.
    wxWindow* focus = wxWindow::FindFocus();
    if (focus) {
        // Trigger KillFocus (most fields update here)
        wxFocusEvent killFocusEvent(wxEVT_KILL_FOCUS, focus->GetId());
        killFocusEvent.SetEventObject(focus);
        focus->GetEventHandler()->ProcessEvent(killFocusEvent);
        
        // Trigger TextEnter (for text fields that rely on Enter)
        wxCommandEvent enterEvent(wxEVT_TEXT_ENTER, focus->GetId());
        enterEvent.SetEventObject(focus);
        focus->GetEventHandler()->ProcessEvent(enterEvent);
    }
    
    // Process any asynchronous updates triggered by the above handlers
    wxYield();

    if (!this->current_preset) return;

    // 2. Defer saving via CallAfter to ensure even slower event handlers are finished.
    //    We capture 'this' safely. Be aware that 'current_preset' pointer stability is reliable 
    //    as long as we don't manipulate the preset vector before this runs.
    SLIC3RAPP->CallAfter([this]() {
        if (!this->current_preset) return;

        // Use a unified Save/Save As dialog.
        wxString prompt = (this->current_preset->default_preset) 
            ? _("Enter a name for the new preset:") 
            : _("Enter a name to save as (leave empty to update current):");

        wxTextEntryDialog dlg(this, prompt, _("Save Preset"));
        
        if (dlg.ShowModal() != wxID_OK) return;

        wxString name = dlg.GetValue();
        name.Trim(true).Trim(false);

        Slic3r::Log::info("GUI", "Save Preset: Input Name='" + name.ToStdString() + "', IsDefault=" + (this->current_preset->default_preset ? "yes" : "no"));

        // Case 1: Empty Name
        if (name.IsEmpty()) {
            if (this->current_preset->default_preset) {
                 // Cannot overwrite default preset with empty name
                 wxMessageBox(_("You cannot overwrite a default preset. Please enter a name to create a new preset."), _("Error"), wxICON_ERROR);
                 return;
            } else {
                 // Update current user preset
                 this->current_preset->save();
                 
                 // Capture name before reloading (which invalidates current_preset)
                 std::string saved_name = this->current_preset->name.ToStdString();

                 // Reload from disk to verify save and update UI
                 this->load_presets();

                 // Re-select the saved preset
                 int n = this->_presets_choice->FindString(saved_name);
                 if (n != wxNOT_FOUND) {
                     this->_presets_choice->SetSelection(n);
                     this->_on_select_preset();
                 }
                 return;
            }
        }

        // Case 2: Name Provided
        
        // Check if name is identical to current (Update)
        if (!this->current_preset->default_preset && name == this->current_preset->name) {
             this->current_preset->save();
             int sel = this->_presets_choice->GetSelection();
             if (sel != wxNOT_FOUND) {
                 this->_presets_choice->SetString(sel, this->current_preset->dropdown_name());
             }
             return;
        }

        // Case 3: New Name (Save As)
        Preset new_preset(false, name, this->current_preset->group);
        
        // Deep copy of configs
        new_preset._config = std::make_shared<Config>(*this->current_preset->_config);
        new_preset._dirty_config = std::make_shared<Config>(*this->current_preset->_dirty_config);

        if (new_preset.save_as(name, {})) {
            // Safe to access group before push_back potentially invalidates current_preset
            auto group_idx = static_cast<int>(this->current_preset->group);
            
            // Add to Global App Presets
             SLIC3RAPP->presets.at(group_idx).push_back(new_preset);
             
             // Reload the list
             this->load_presets();
             
             // Select the new preset
             int n = this->_presets_choice->FindString(new_preset.dropdown_name());
             if (n != wxNOT_FOUND) {
                 this->_presets_choice->SetSelection(n);
                 this->_on_select_preset();
             }
        }
    });
}

void PresetEditor::_on_select_preset(bool force) {
    int sel = this->_presets_choice->GetSelection();
    if (sel < 0) return;
    
    auto& presets = SLIC3RAPP->presets.at(this->typeId());
    if (sel >= presets.size()) return;
    
    Preset* p = &presets[sel];
    // Hack for shared_ptr
    this->current_preset = std::shared_ptr<Preset>(p, [](Preset*){});
    
    // Toggle delete button
    if (this->_btn_delete_preset) {
        this->_btn_delete_preset->Enable(!p->default_preset);
    }    
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
    
    // Retrieve dirty options to pass to update logic
    std::vector<std::string> dirty_keys;
    if (this->current_preset) {
        dirty_keys = this->current_preset->dirty_options();
    }
    
    for (auto* page : _pages) {
         page->update_options(&this->config->config(), dirty_keys);
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
