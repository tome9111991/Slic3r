#include "PresetChooser.hpp"
#include "misc_ui.hpp"
#include "Theme/ThemeManager.hpp"
#include "Plater.hpp"

namespace Slic3r { namespace GUI {

PresetChooser::PresetChooser(wxWindow* parent, std::weak_ptr<Print> print) : PresetChooser(parent, print, SLIC3RAPP->settings(), SLIC3RAPP->presets) {}

PresetChooser::PresetChooser(wxWindow* parent, std::weak_ptr<Print> print, Settings* external_settings, preset_store& external_presets) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, ""),
    _local_sizer(new wxFlexGridSizer(3,3,1,2)), _parent(parent), _settings(external_settings), _print(print), _presets(external_presets)
{
    // Apply panel background
    if (ThemeManager::IsDark()) {
        this->SetBackgroundColour(ThemeManager::GetColors().bg);
    }

    _local_sizer->AddGrowableCol(1, 1);
    _local_sizer->SetFlexibleDirection(wxHORIZONTAL);

    for (auto group : { preset_t::Print, preset_t::Material, preset_t::Printer }) {
        wxString name = "";
        switch(group) {
            case preset_t::Print:
                name << _("Print settings:");
                break;
            case preset_t::Material:
                name << _("Material:");
                break;
            case preset_t::Printer:
                name << _("Printer:");
                break;
            default:
                break;
        }
        auto* text {new wxStaticText(this, wxID_ANY, name, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT)};
        text->SetFont(_settings->small_font());
        if (ThemeManager::IsDark()) {
            text->SetForegroundColour(*wxWHITE);
        }

        m_labels.push_back(text);
        
        auto* choice = new ThemedSelect(this, wxID_ANY, wxArrayString(), wxDefaultPosition, wxDefaultSize);
        this->preset_choosers[get_preset(group)].push_back(choice);
        
        // Settings button
        auto* settings_btn = new ThemedButton(this, wxID_ANY, "", wxDefaultPosition, wxSize(30, 30));
        settings_btn->SetBitmap(get_bmp_bundle("cog.svg"));
        m_settings_buttons.push_back(settings_btn);
        
        settings_btn->Bind(wxEVT_BUTTON, [this, group](wxCommandEvent& e) {
             if (auto plater = dynamic_cast<Plater*>(this->GetParent())) {
                 plater->show_preset_editor(group, 0); 
             }
        });

        this->_local_sizer->Add(text, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        this->_local_sizer->Add(choice, 1, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxBOTTOM, 0);
        this->_local_sizer->Add(settings_btn, 0, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxLEFT, 3);
        
        // setup listener. 
        // On a combobox event, puts a call to _on_change_combobox() on the evt_idle stack.
        // ThemedSelect fires wxEVT_COMBOBOX
        choice->Bind(wxEVT_COMBOBOX, 
            [=](wxCommandEvent& e) { 
                wxTheApp->CallAfter([=]() { this->_on_change_combobox(group, choice);} );
            });
    }

    this->SetSizer(_local_sizer);
}

void PresetChooser::load(std::array<Presets, preset_types> presets) {

    wxString selected_printer_name {""};
    for (const auto& group : { preset_t::Printer, preset_t::Material, preset_t::Print }) {
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
        std::vector<std::string> preset_names {};
        __chooser_names[get_preset(group)].clear();
        // populate the chooser
        for (auto* chooser : this->preset_choosers[get_preset(group)]) {
            chooser->Clear();
            assert(chooser->GetCount() == 0);
            for (auto preset : current_list) {
                wxBitmapBundle bitmap;
                switch (group) {
                    case preset_t::Print:
                        bitmap = get_bmp_bundle("cog.svg");
                        break;
                    case preset_t::Material: 
                        if (auto config =  preset.config().lock()) {
                            if (preset.default_preset || !config->has("filament_colour"))
                                bitmap = get_bmp_bundle("spool.svg");
                        } else { // fall back if for some reason the config is dead.
                            bitmap = get_bmp_bundle("spool.svg");
                        }
                        break;
                    case preset_t::Printer: 
                        bitmap = get_bmp_bundle("printer_empty.svg");
                        break;
                    default: break;
                }
                chooser->Append(preset.name, bitmap);
                __chooser_names[get_preset(group)].push_back(preset.name);
            }
            assert(chooser->GetCount() == current_list.size());

            // Apply default options from settings
            bool updated_from_settings = false;
            if (settings_defaults.size() > i) { // only apply if there is a value from Settings
                updated_from_settings = this->select_preset_by_name(settings_defaults.at(i), chooser);
            }

            if (!updated_from_settings) // default
                chooser->SetSelection(0);

            wxString selected_preset { chooser->GetString(chooser->GetSelection()) };
            if (group == preset_t::Printer) {
                selected_printer_name = selected_preset;
            }

            ++i;
        }
        this->_update_preset_settings(group);
    }
}

bool PresetChooser::select_preset_by_name(wxString name, preset_t group, size_t index = 0) {
    auto& ps_list = this->preset_choosers.at(get_preset(group));
    bool updated = false;
    if (ps_list.size() > index) {
        updated = select_preset_by_name(name, ps_list.at(index));
    }
    if (updated)
        this->_on_select_preset(group);
    return updated;
}

bool PresetChooser::select_preset_by_name(wxString name, ThemedSelect* chooser) {
    if (chooser->GetCount() == 0) return false;
    
    // FindString replacement
    int index = wxNOT_FOUND;
    for(size_t i=0; i<chooser->GetCount(); ++i) {
        if(chooser->GetString(i) == name) {
            index = (int)i;
            break;
        }
    }

    if (index != wxNOT_FOUND) {
        chooser->SetSelection(index);
        return true;
    }
    return false;
}

void PresetChooser::_update_preset_settings(preset_t preset) {
    auto& settings_presets {_settings->default_presets.at(get_preset(preset))};
    settings_presets.clear(); // make sure previous contents are deconstructed
    settings_presets = this->_get_selected_presets(preset);

}

void PresetChooser::_on_select_preset(preset_t preset) {
    // update settings store
    this->_update_preset_settings(preset);
    // save settings
    _settings->save_settings();
    if (preset == preset_t::Printer) {
        this->load(); // reload print/filament settings to honor compatible printers
    }
}

bool PresetChooser::prompt_unsaved_changes() {
    return true;
}

std::vector<wxString> PresetChooser::_get_selected_presets(preset_t group) const {
        const auto& choosers { this->preset_choosers[get_preset(group)] };
        std::vector<wxString> selected;
        selected.reserve(choosers.size());

        for (auto* chooser :  choosers) {
            selected.push_back(chooser->GetString(chooser->GetSelection()));
        }
        return selected;
}

wxString PresetChooser::_get_selected_preset(preset_t group, size_t index) const {
    auto selected { this->_get_selected_presets(group) };
    if (index > selected.size()) { return wxString(""); }
    return selected.at(index);
}
void PresetChooser::_on_change_combobox(preset_t preset, ThemedSelect* choice) {
    
    // Prompt for unsaved changes and undo selections if cancelled and return early
    // Callback to close preset editor tab, close editor tabs, reload presets.
    //
    if (!this->prompt_unsaved_changes()) return;
    wxTheApp->CallAfter([this,preset]()
    {
        this->_on_select_preset(preset);
        // reload presets; removes the modified mark
        this->load();
    });
    /*
    sub _on_change_combobox {
    my ($self, $group, $choice) = @_;
    
    if (0) {
        # This code is disabled because wxPerl doesn't provide GetCurrentSelection
        my $current_name = $self->{preset_choosers_names}{$choice}[$choice->GetCurrentSelection];
        my $current = first { $_->name eq $current_name } @{wxTheApp->presets->{$group}};
        if (!$current->prompt_unsaved_changes($self)) {
            # Restore the previous one
            $choice->SetSelection($choice->GetCurrentSelection);
            return;
        }
    } else {
        return 0 if !$self->prompt_unsaved_changes;
    }
    wxTheApp->CallAfter(sub {
        # Close the preset editor tab if any
        if (exists $self->GetFrame->{preset_editor_tabs}{$group}) {
            my $tabpanel = $self->GetFrame->{tabpanel};
            $tabpanel->DeletePage($tabpanel->GetPageIndex($self->GetFrame->{preset_editor_tabs}{$group}));
            delete $self->GetFrame->{preset_editor_tabs}{$group};
            $tabpanel->SetSelection(0); # without this, a newly created tab will not be selected by wx
        }
        
        $self->_on_select_preset($group);
        
        # This will remove the "(modified)" mark from any dirty preset handled here.
        $self->load_presets;
    });
}
*/
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

    for (auto* text : m_labels) {
        text->SetForegroundColour(dark ? *wxWHITE : *wxBLACK);
    }

    for (auto& group : preset_choosers) {
        for (auto* choice : group) {
            // choice->UpdateTheme(); // If ThemedSelect exposes it? It repaints nicely on usage.
            choice->Refresh();
        }
    }

    for (auto* btn : m_settings_buttons) {
        btn->SetBitmap(get_bmp_bundle("cog.svg")); 
        btn->Refresh();
    }
    
    this->Refresh();
}

}} // Slic3r::GUI
