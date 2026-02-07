#include "PresetChooser.hpp"
#include "misc_ui.hpp"
#include "Theme/ThemeManager.hpp"
#include "Plater.hpp"
#include "../Widgets/ThemedMenu.hpp"
#include "../Widgets/ThemedMenuPopup.hpp"

namespace Slic3r { namespace GUI {

PresetChooser::PresetChooser(wxWindow* parent, std::weak_ptr<Print> print) : PresetChooser(parent, print, SLIC3RAPP->settings(), SLIC3RAPP->presets) {}

PresetChooser::PresetChooser(wxWindow* parent, std::weak_ptr<Print> print, Settings* external_settings, preset_store& external_presets) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxCLIP_CHILDREN, ""),
    _local_sizer(new wxBoxSizer(wxVERTICAL)), _parent(parent), _settings(external_settings), _print(print), _presets(external_presets)
{
    // Apply panel background
    if (ThemeManager::IsDark()) {
        this->SetBackgroundColour(ThemeManager::GetColors().bg);
    }

    for (auto group : { preset_t::Printer, preset_t::Print, preset_t::Material }) {
        wxString name = "";
        wxString icon = "";
        switch(group) {
            case preset_t::Print:
                name = _("Process");
                icon = "cog.svg";
                break;
            case preset_t::Material:
                name = _("Filament");
                icon = "spool.svg";
                break;
            case preset_t::Printer:
                name = _("Printer");
                icon = "printer_empty.svg";
                break;
            default:
                break;
        }
        
        auto* section = new PresetSection(this, name, icon);
        this->preset_sections[get_preset(group)].push_back(section);
        
        size_t index = this->preset_sections[get_preset(group)].size() - 1;

        // Connect the inner selector event
        section->GetSelector()->Bind(wxEVT_COMBOBOX, [this, group, index, section](wxCommandEvent& e) {
             wxTheApp->CallAfter([this, group, index, section]() {
                 this->_on_change_combobox(group, index, section->GetValue());
             });
        });

        section->on_settings_click = [this, group]() {
             if (auto plater = dynamic_cast<Plater*>(this->GetParent())) {
                 plater->show_preset_editor(group, 0); 
             }
        };

        section->on_edit_click = [this, group]() {
             if (auto plater = dynamic_cast<Plater*>(this->GetParent())) {
                 plater->show_preset_editor(group, 0); 
             }
        };

        this->_local_sizer->Add(section, 0, wxEXPAND | wxBOTTOM, 5);
    }

    this->SetSizer(_local_sizer);
}

void PresetChooser::load(std::array<Presets, preset_types> presets) {

    wxString selected_printer_name {""};
    for (const auto& group : { preset_t::Printer, preset_t::Print, preset_t::Material }) {
        auto current_list = presets.at(get_preset(group));
        // Filter out profiles not compatible with this printer
        if (group != preset_t::Printer) {
            current_list = grep(presets.at(get_preset(group)), [selected_printer_name] (const Preset& x) -> bool { return x.compatible(selected_printer_name); });
        }

        // show default names if no other presets visible.
        if (current_list.size() > 1) {
            current_list = grep(current_list, [] (const Preset& x) -> bool { return !x.default_preset; });
        }

        // Read the current defaults from the settings file
        const auto& settings_defaults {_settings->default_presets.at(get_preset(group))};

        size_t i {0};
        __chooser_names[get_preset(group)].clear();
        
        // populate the chooser sections
        for (auto* section : this->preset_sections[get_preset(group)]) {
            auto* selector = section->GetSelector();
            selector->Clear();

            for (auto preset : current_list) {
                selector->Append(preset.name, wxBitmapBundle());
                __chooser_names[get_preset(group)].push_back(preset.name);
            }

            // Apply default options from settings
            wxString selected_name = "";
            if (settings_defaults.size() > i) { 
                selected_name = settings_defaults.at(i);
            }

            if (selected_name.IsEmpty() && selector->GetCount() > 0) {
                selected_name = selector->GetString(0);
            }

            section->SetValue(selected_name);
            
            if (group == preset_t::Printer) {
                selected_printer_name = selected_name;
            }

            ++i;
        }
        this->_update_preset_settings(group);
    }
}

bool PresetChooser::select_preset_by_name(wxString name, preset_t group, size_t index) {
    auto& ps_list = this->preset_sections.at(get_preset(group));
    if (ps_list.size() > index) {
        ps_list.at(index)->SetValue(name);
        this->_on_select_preset(group);
        return true;
    }
    return false;
}

bool PresetChooser::select_preset_by_name(wxString name, ThemedSelect* chooser) {
    return false;
}

void PresetChooser::_update_preset_settings(preset_t preset) {
    auto& settings_presets {_settings->default_presets.at(get_preset(preset))};
    settings_presets.clear();
    settings_presets = this->_get_selected_presets(preset);
}

void PresetChooser::_on_select_preset(preset_t preset) {
    this->_update_preset_settings(preset);
    _settings->save_settings();
    if (preset == preset_t::Printer) {
        this->load(); 
    }
    if (this->on_change) {
        this->on_change(preset);
    }
}

bool PresetChooser::prompt_unsaved_changes() {
    return true;
}

std::vector<wxString> PresetChooser::_get_selected_presets(preset_t group) const {
        const auto& sections { this->preset_sections[get_preset(group)] };
        std::vector<wxString> selected;
        selected.reserve(sections.size());

        for (auto* section :  sections) {
            selected.push_back(section->GetValue());
        }
        return selected;
}

wxString PresetChooser::_get_selected_preset(preset_t group, size_t index) const {
    auto selected { this->_get_selected_presets(group) };
    if (index >= selected.size()) { return wxString(""); }
    return selected.at(index);
}

void PresetChooser::_on_change_combobox(preset_t preset, size_t index, const wxString& selection) {
    if (!this->prompt_unsaved_changes()) return;
    
    if (index < this->preset_sections[get_preset(preset)].size()) {
        this->preset_sections[get_preset(preset)][index]->SetValue(selection);
    }

    wxTheApp->CallAfter([this,preset]()
    {
        this->_on_select_preset(preset);
        this->load();
    });
}

wxSize PresetChooser::DoGetBestSize() const {
    if (GetSizer()) {
        return GetSizer()->GetMinSize();
    }
    double scale = GetContentScaleFactor();
    return wxSize(320 * scale, 300 * scale);
}

void PresetChooser::UpdateTheme()
{
    bool dark = ThemeManager::IsDark();
    auto colors = ThemeManager::GetColors();

    if (dark) {
        this->SetBackgroundColour(colors.bg);
    } else {
        this->SetBackgroundColour(wxNullColour);
    }

    for (auto group : { preset_t::Printer, preset_t::Print, preset_t::Material }) {
        for (auto* section : this->preset_sections[get_preset(group)]) {
            section->UpdateTheme();
        }
    }
    
    this->Refresh();
}

}} // Slic3r::GUI
