#include "OptionsGroup.hpp"
#include "OptionsGroup/Field.hpp"
#include "Theme/ThemeManager.hpp"
#include "misc_ui.hpp" /* For get_bmp_bundle */
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/gbsizer.h>
#include "libslic3r/PrintConfig.hpp"
#include "Log.hpp"
#include "GUI.hpp"

namespace Slic3r { namespace GUI {

Option OptionsGroup::get_option(const t_config_option_key& opt_key) {
    if (print_config_def.has(opt_key)) {
        const ConfigOptionDef& def = print_config_def.get(opt_key);
        return Option(opt_key, def.type, *def.default_value);
    }
    ConfigOptionDef def;
    def.label = opt_key;
    ConfigOptionString dummy(""); 
    return Option(opt_key, coString, dummy);
}

void OptionsGroup::append_single_option_line(const t_config_option_key& opt_key) {
    auto* field = this->build_field(opt_key);
    if (!field) return;
    
    _fields[opt_key] = std::unique_ptr<UI_Field>(field);

    int start_col = this->show_quick_setting_toggles ? 1 : 0;
    int row = m_row_count++;

    if (this->show_quick_setting_toggles) {
        if (!this->star_filled.IsOk()) {
            this->star_filled = get_bmp_bundle("star_filled.svg", 8);
            this->star_empty = get_bmp_bundle("star_empty.svg", 8);
        }

        auto* btn_quick = new ThemedButton(parent, wxID_ANY, "", wxDefaultPosition, wxSize(12, 12));
        btn_quick->SetBitmap(ui_settings->is_quick_setting(opt_key) ? this->star_filled : this->star_empty);
        btn_quick->Bind(wxEVT_BUTTON, [this, opt_key, btn_quick](wxCommandEvent&) {
            if (this->on_quick_setting_change) {
                 this->on_quick_setting_change(opt_key, true); 
            }
        });
        
        // 1. Star Column
        _quick_toggles[opt_key] = btn_quick;
        grid_sizer->Add(btn_quick, wxGBPosition(row, 0), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL);
    }

    std::string label_text = opt_key;
    std::string unit_text = "";

    ConfigOptionDef def;
    if (_options.count(opt_key)) def = _options[opt_key]->desc;
    else if (print_config_def.has(opt_key)) def = print_config_def.get(opt_key);

    if (!def.label.empty()) label_text = def.label;
    if (def.full_label.empty()) def.full_label = label_text;

    std::string extra_unit_text = "";
    // Unit extraction
    if (!def.sidetext.empty()) {
        unit_text = def.sidetext;
    } else {
        size_t open_paren = label_text.find_last_of('(');
        size_t close_paren = label_text.find_last_of(')');
        if (open_paren != std::string::npos && close_paren != std::string::npos && close_paren > open_paren) {
             unit_text = label_text.substr(open_paren + 1, close_paren - open_paren - 1);
             label_text = label_text.substr(0, open_paren);
             while (!label_text.empty() && (isspace(label_text.back()) || label_text.back() == '(' )) label_text.pop_back();
        }
    }

    // Special handling for "zero to disable" - move it to a label instead of suffix if possible
    size_t zero_pos = unit_text.find("zero to disable");
    if (zero_pos == std::string::npos) zero_pos = unit_text.find("Zero to disable");

    if (zero_pos != std::string::npos) {
        size_t start = zero_pos;
        size_t end = zero_pos + 15;
        
        if (start > 0 && unit_text[start-1] == '(') start--;
        if (end < unit_text.length() && unit_text[end] == ')') end++;

        extra_unit_text = unit_text.substr(start, end - start);
        unit_text.erase(start, end - start);
        // Clean up remaining spaces or opening parens
        while (!unit_text.empty() && (isspace(unit_text.back()) || unit_text.back() == '(' )) unit_text.pop_back();
    }
    
    // Set suffix on field (only the remaining part like "mm")
    if (!unit_text.empty()) {
        field->set_suffix(wxString::FromUTF8(unit_text));
    }

    int label_row = row;
    auto* label = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8(label_text + ":"));
    if (ThemeManager::IsDark()) label->SetForegroundColour(*wxWHITE);
    else label->SetForegroundColour(*wxBLACK);
    
    int left_border = 0;
    if (label_text.find("↳") != std::string::npos) {
        left_border = 15;
    }

    // 2. Label Column
    grid_sizer->Add(label, wxGBPosition(label_row, start_col), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL | wxLEFT, left_border);
    _labels[opt_key] = label;
    _label_to_keys[label].push_back(opt_key);

    if (this->get_option(opt_key).desc.type == coBool) {
        // 3. Field Column
        grid_sizer->Add(field->get_window(), wxGBPosition(label_row, start_col + 1), wxGBSpan(1, 1), wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    } else if (this->get_option(opt_key).desc.type == coEnum) {
        field->get_window()->SetMinSize(wxSize(180, -1)); 
        grid_sizer->Add(field->get_window(), wxGBPosition(label_row, start_col + 1), wxGBSpan(1, 1), wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    } else if (def.multiline) {
        field->get_window()->SetMinSize(wxSize(200, 150));
        // Move to NEXT row for multi-line text areas
        int field_row = m_row_count++;
        // Multi-line spans full width (Star + Label + Field + Unit + Grow columns)
        int total_cols = this->show_quick_setting_toggles ? 5 : 4;
        grid_sizer->Add(field->get_window(), wxGBPosition(field_row, 0), wxGBSpan(1, total_cols), wxEXPAND | wxTOP | wxBOTTOM, 5);
    } else {
        field->get_window()->SetMinSize(wxSize(180, -1)); 
        grid_sizer->Add(field->get_window(), wxGBPosition(label_row, start_col + 1), wxGBSpan(1, 1), wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    }

    // If unit is NOT handled by suffix (or we want a fallback label), add it
    // For now, if we set suffix, we don't need the extra label unless it's a coBool? 
    // But coBool doesn't have units.
    // Let's only add the unit label if it's NOT a numeric field that already took the suffix.
    bool show_unit_label = !extra_unit_text.empty() || (!unit_text.empty() && (def.type == coBool || def.multiline));

    if (show_unit_label) {
        std::string final_unit = unit_text;
        if (def.type != coBool && !def.multiline) final_unit = ""; // Suffix took it
        
        if (!extra_unit_text.empty()) {
            if (!final_unit.empty()) final_unit += " ";
            final_unit += extra_unit_text;
        }

        auto* unit_lbl = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8(final_unit));
        unit_lbl->SetFont(ThemeManager::GetFont(ThemeManager::FontSize::Tiny));
        if (ThemeManager::IsDark()) unit_lbl->SetForegroundColour(wxColour(150, 150, 150));
        else unit_lbl->SetForegroundColour(wxColour(100, 100, 100));
        // 4. Unit Column
        grid_sizer->Add(unit_lbl, wxGBPosition(label_row, start_col + 2), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
        _units[opt_key] = unit_lbl;
    }
}

void OptionsGroup::append_line(const GUI::Line& line) {
    int start_col = this->show_quick_setting_toggles ? 1 : 0;
    int row = m_row_count++;

    if (this->show_quick_setting_toggles) {
        // 1. Star column (Spacer)
        grid_sizer->Add(12, 12, wxGBPosition(row, 0));
    }

    // 2. Label column
    wxStaticText* label = nullptr;
    if (!line.label.empty()) {
        label = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8(line.label + ":"));
        if (ThemeManager::IsDark()) label->SetForegroundColour(*wxWHITE);
        else label->SetForegroundColour(*wxBLACK);
        grid_sizer->Add(label, wxGBPosition(row, start_col), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    }
    
    // 3. Field column (can contain multiple fields)
    auto* line_sizer = new wxBoxSizer(wxHORIZONTAL);
    for (const auto& opt : line.options) {
        if (label) {
            _labels[opt.opt_id] = label;
            _label_to_keys[label].push_back(opt.opt_id);
        }
        _options[opt.opt_id] = std::make_unique<Option>(opt);
        auto* field = this->build_field(opt.opt_id);
        if (field) {
            _fields[opt.opt_id] = std::unique_ptr<UI_Field>(field);
            
            // Handle unit for each field in the line
            std::string unit = "";
            std::string extra_unit = "";
            if (!opt.desc.sidetext.empty()) {
                unit = opt.desc.sidetext;
            } else if (!opt.desc.label.empty()) {
                size_t open_paren = opt.desc.label.find_last_of('(');
                size_t close_paren = opt.desc.label.find_last_of(')');
                if (open_paren != std::string::npos && close_paren != std::string::npos && close_paren > open_paren) {
                    unit = opt.desc.label.substr(open_paren + 1, close_paren - open_paren - 1);
                }
            }

            // Special handling for "zero to disable"
            size_t zero_pos = unit.find("zero to disable");
            if (zero_pos == std::string::npos) zero_pos = unit.find("Zero to disable");

            if (zero_pos != std::string::npos) {
                size_t start = zero_pos;
                size_t end = zero_pos + 15;
                if (start > 0 && unit[start-1] == '(') start--;
                if (end < unit.length() && unit[end] == ')') end++;

                extra_unit = unit.substr(start, end - start);
                unit.erase(start, end - start);
                while (!unit.empty() && (isspace(unit.back()) || unit.back() == '(' )) unit.pop_back();
            }

            if (!unit.empty()) {
                field->set_suffix(wxString::FromUTF8(unit));
            }

            if (opt.desc.type == coBool) {
                line_sizer->Add(field->get_window(), 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
            } else {
                field->get_window()->SetMinSize(wxSize(100, -1));
                line_sizer->Add(field->get_window(), 1, wxEXPAND | wxRIGHT, 5);
            }

            if (!extra_unit.empty()) {
                auto* unit_lbl = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8(extra_unit));
                unit_lbl->SetFont(ThemeManager::GetFont(ThemeManager::FontSize::Tiny));
                if (ThemeManager::IsDark()) unit_lbl->SetForegroundColour(wxColour(150, 150, 150));
                else unit_lbl->SetForegroundColour(wxColour(100, 100, 100));
                line_sizer->Add(unit_lbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
            }
        }
    }
    grid_sizer->Add(line_sizer, wxGBPosition(row, start_col + 1), wxGBSpan(1, 1), wxEXPAND | wxALIGN_CENTER_VERTICAL);
}

ConfigOption* OptionsGroup::get_field(const t_config_option_key& opt_key) {
    return nullptr; 
}

void OptionsGroup::update_options(const ConfigBase* config, const std::vector<std::string>& dirty_keys) {
    _dirty_states.clear();
    for (const auto& key : dirty_keys) {
        _dirty_states[key] = true;
    }

    for (auto& [key, field] : _fields) {
        if (config->has(key)) {
            auto* opt = config->option(key);
            if (!opt) continue;
            
            const ConfigOptionDef* def = nullptr;
            if (_options.count(key)) {
                def = &_options[key]->desc;
            } else if (print_config_def.has(key)) {
                def = &print_config_def.get(key);
            }
            
            if (def) {
                 try {
                     switch (def->type) {
                         case coBool: field->set_value(opt->getBool()); break;
                         case coInt: field->set_value(opt->getInt()); break;
                         case coFloat: field->set_value(opt->getFloat()); break;
                         case coString: field->set_value(opt->getString()); break;
                         case coEnum: {
                                 field->set_value(opt->serialize());
                                 break;
                         }
                         default: 
                             field->set_value(opt->serialize());
                             break;
                     }
                 } catch (...) {
                 }
            }

            bool is_dirty = std::find(dirty_keys.begin(), dirty_keys.end(), key) != dirty_keys.end();
            this->set_dirty_status(key, is_dirty);

            if (_quick_toggles.count(key)) {
                this->set_quick_setting_status(key, ui_settings->is_quick_setting(key));
            }
        }
    }
}

void OptionsGroup::set_quick_setting_status(const std::string& opt_key, bool active) {
    if (_quick_toggles.count(opt_key)) {
        if (auto* btn = dynamic_cast<ThemedButton*>(_quick_toggles[opt_key])) {
            if (!this->star_filled.IsOk()) {
                 this->star_filled = get_bmp_bundle("star_filled.svg", 8);
                 this->star_empty = get_bmp_bundle("star_empty.svg", 8);
            }
            btn->SetBitmap(active ? this->star_filled : this->star_empty);
        }
    }
}

void OptionsGroup::set_dirty_status(const std::string& opt_key, bool is_dirty) {
    _dirty_states[opt_key] = is_dirty;

    if (_labels.count(opt_key)) {
        wxStaticText* lbl = _labels[opt_key];
        
        bool any_dirty = false;
        if (_label_to_keys.count(lbl)) {
            for (const auto& key : _label_to_keys[lbl]) {
                if (_dirty_states.count(key) && _dirty_states[key]) {
                    any_dirty = true;
                    break;
                }
            }
        } else {
            any_dirty = is_dirty;
        }

        if (any_dirty) {
            lbl->SetForegroundColour(wxColour(255, 128, 0)); // Orange
        } else {
            if (ThemeManager::IsDark()) lbl->SetForegroundColour(*wxWHITE);
            else lbl->SetForegroundColour(*wxBLACK);
        }
        lbl->Refresh();
    }
}

UI_Field* OptionsGroup::build_field(const t_config_option_key& opt_key) {
    ConfigOptionDef def;
    if (_options.count(opt_key)) {
        def = _options[opt_key]->desc;
    } else if (print_config_def.has(opt_key)) {
        def = print_config_def.get(opt_key);
    } else {
        def.label = opt_key;
        def.type = coString;
    }
    
    if (def.gui_type == "color") {
        auto* f = new UI_Color(parent, def);
        f->on_change = [this, opt_key](const std::string&, std::string val) {
            if (this->on_change) {
                this->on_change(opt_key, val);
                this->on_change(opt_key, boost::any()); // Visual update
            }
        };
        return f;
    }

    UI_Field* field = nullptr;
    switch (def.type) {
        case coFloat:
        case coFloats:
        case coString:
        case coFloatOrPercent:
        case coPercent: {
            auto* f = new UI_TextCtrl(parent, def);
            f->on_change = [this, opt_key](const std::string&, std::string val) { 
               if (this->on_change) this->on_change(opt_key, val);
            };
            f->on_change_final = [this, opt_key](const std::string&) {
               // This is where we trigger label color update
               if (this->on_change) this->on_change(opt_key, boost::any()); 
            };
            field = f;
            break;
        }
        case coInt: {
            auto* f = new UI_SpinCtrl(parent, def);
            f->on_change = [this, opt_key](const std::string&, int val) { 
               if (this->on_change) this->on_change(opt_key, val);
            };
            f->on_change_final = [this, opt_key](const std::string&) {
               if (this->on_change) this->on_change(opt_key, boost::any());
            };
            field = f;
            break;
        }
        case coBool: {
            auto* f = new UI_Checkbox(parent, def);
            f->on_change = [this, opt_key](const std::string&, bool val) { 
               if (this->on_change) {
                   this->on_change(opt_key, val);
                   this->on_change(opt_key, boost::any()); // Trigger visual update
               }
            };
            field = f;
            break;
        }
        case coEnum: {
            auto* f = new UI_Choice(parent, def);
             f->on_change = [this, opt_key](const std::string&, std::string val) { 
               if (this->on_change) {
                   this->on_change(opt_key, val);
                   this->on_change(opt_key, boost::any()); // Trigger visual update
               }
            };
            field = f;
            break;
        }
        default: {
            auto* f = new UI_TextCtrl(parent, def);
             f->on_change = [this, opt_key](const std::string&, std::string val) { 
               if (this->on_change) {
                   this->on_change(opt_key, val);
                   this->on_change(opt_key, boost::any());
               }
            };
            field = f;
        }
    }
    return field;
}

void OptionsGroup::set_sizer(wxBoxSizer* s) {
    sizer = s;
    if (auto* ss = dynamic_cast<wxStaticBoxSizer*>(s)) {
        parent = ss->GetStaticBox();
    }

    int grow_col = this->show_quick_setting_toggles ? 4 : 3;
    grid_sizer = new wxGridBagSizer(5, 5); 
    grid_sizer->SetCols(grow_col + 1); // Explicitly set column count to avoid AddGrowableCol asserts
    grid_sizer->AddGrowableCol(grow_col, 1); // Only the last dummy column grows to push content to left
    
    if (this->sizer)
        this->sizer->Add(grid_sizer, 0, wxEXPAND | wxALL, 5);
}

}} // namespace Slic3r::GUI
