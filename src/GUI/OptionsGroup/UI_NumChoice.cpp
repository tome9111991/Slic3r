#include "OptionsGroup/Field.hpp"
#include "misc_ui.hpp"
#include <typeinfo>

namespace Slic3r { namespace GUI {

void UI_NumChoice::set_value(boost::any value) {
    const bool show_value { std::regex_search(this->opt.gui_flags, show_value_flag) };

    if (value.type() == typeid(int)) {
        this->_set_value(boost::any_cast<int>(value), show_value); return;
    } else if (value.type() == typeid(double)) {
        this->_set_value(boost::any_cast<double>(value), show_value); return;
    } else if (value.type() == typeid(float)) {
        this->_set_value(boost::any_cast<float>(value), show_value); return;
    } else if (value.type() == typeid(std::string)) {
        this->_set_value(boost::any_cast<std::string>(value)); return;
    } else if (value.type() == typeid(wxString)) {
        this->_set_value(boost::any_cast<wxString>(value).ToStdString()); return;
    } else {
        Slic3r::Log::warn(this->LogChannel(), LOG_WSTRING(wxString("Unsupported type ") + value.type().name() + wxString(" for set_value") ));
    }
}

void UI_NumChoice::_set_value(int value, bool show_value) {
    auto& vlist {this->opt.enum_values};
    auto& llist {this->opt.enum_labels};

    if (_choice) {
        if (show_value) {
            this->_choice->SetValue(std::to_string(value)); 
        } else {
            if (vlist.size() > 0) {
                auto result {std::find(vlist.cbegin(), vlist.cend(), std::to_string(value))};
                if (result != vlist.cend()) {
                    auto value_idx {std::distance(vlist.cbegin(), result)};
                    this->_choice->SetSelection(value_idx);
                    this->disable_change_event = false;
                    return;
                }
            } else if (llist.size() > 0 && static_cast<size_t>(value) < llist.size()) {
                this->_choice->SetValue(wxString(llist.at(value)));
                this->disable_change_event = false;
                return;
            }
            this->_choice->SetValue(wxString(std::to_string(value)));
        }
    } else if (_combo) {
        if (show_value) {
            this->_combo->ChangeValue(std::to_string(value)); 
        } else {
            if (vlist.size() > 0) {
                auto result {std::find(vlist.cbegin(), vlist.cend(), std::to_string(value))};
                if (result != vlist.cend()) {
                    auto value_idx {std::distance(vlist.cbegin(), result)};
                    this->_combo->SetSelection(value_idx);
                    this->disable_change_event = false;
                    return;
                }
            } else if (llist.size() > 0 && static_cast<size_t>(value) < llist.size()) {
                this->_combo->SetValue(wxString(llist.at(value)));
                this->disable_change_event = false;
                return;
            }
            this->_combo->SetValue(wxString(std::to_string(value)));
        }
    }
    this->disable_change_event = false;
}

void UI_NumChoice::_set_value(double value, bool show_value) {
    if (show_value) {
        if (_choice) this->_choice->SetValue(std::to_string(value));
        else if (_combo) this->_combo->ChangeValue(std::to_string(value));
    }
}

void UI_NumChoice::_set_value(std::string value) {
    if (_choice) this->_choice->SetValue(value);
    else if (_combo) this->_combo->ChangeValue(value);
}

std::string UI_NumChoice::get_string() { 
    if (opt.enum_values.size() > 0) {
        int idx = wxNOT_FOUND;
        wxString val = "";
        if (_choice) {
            idx = _choice->GetSelection();
            val = _choice->GetValue();
        } else if (_combo) {
            idx = _combo->GetSelection();
            val = _combo->GetValue();
        }

        if (idx != wxNOT_FOUND) return this->opt.enum_values.at(idx);
    }
    if (_choice) return _choice->GetValue().ToStdString();
    if (_combo) return _combo->GetValue().ToStdString();
    return "";
}

UI_NumChoice::UI_NumChoice(wxWindow* parent, Slic3r::ConfigOptionDef _opt, wxWindowID id) : UI_Window(parent, _opt) {
    int style {0};
    style |= wxTE_PROCESS_ENTER;
    
    bool is_open = (opt.gui_type == "select_open"s);

    /// Load the values
    auto values {wxArrayString()};
    if (opt.enum_labels.size() > 0)
        for (auto v : opt.enum_labels) values.Add(wxString(v));
    else 
        for (auto v : opt.enum_values) values.Add(wxString(v));

    if (!is_open) {
        _choice = new ThemedSelect(parent, id, values, wxDefaultPosition, _default_size());
        window = _choice;
    } else {
        _combo = new wxComboBox(parent, id, 
                (opt.default_value != nullptr ? opt.default_value->serialize() : ""),
                wxDefaultPosition, _default_size(), values, style);
        window = _combo;
    }

    this->set_value(opt.default_value != nullptr ? opt.default_value->serialize() : "");

    // Event handler
    auto pickup = [this](wxCommandEvent& e) 
    { 
        auto disable_change {this->disable_change_event};
        this->disable_change_event = true;

        int idx = wxNOT_FOUND;
        if (_choice) idx = _choice->GetSelection();
        else if (_combo) idx = _combo->GetSelection();

        wxString lbl {""};
        if (idx != wxNOT_FOUND) {
            if (this->opt.enum_labels.size() > 0 && (unsigned)idx < this->opt.enum_labels.size()) {
                lbl << this->opt.enum_labels.at(idx);
            } else if (this->opt.enum_values.size() > 0 && (unsigned)idx < this->opt.enum_values.size()) {
                lbl << this->opt.enum_values.at(idx);
            } else {
                lbl << idx;
            }
        }

        if (!lbl.IsEmpty()) {
            if (_choice) {
                this->_choice->SetValue(lbl);
            } else if (_combo) {
                this->_combo->CallAfter([this,lbl]() { 
                    auto dce {this->disable_change_event};
                    this->disable_change_event = true;
                    this->_combo->SetValue(lbl);
                    this->disable_change_event = dce;
                });
            }
        }

        this->disable_change_event = disable_change;
        this->_on_change("");
    };

    window->Bind(wxEVT_COMBOBOX, pickup);
    if (_combo) {
        _combo->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent& e) { this->set_value(this->_combo->GetValue()); this->_on_change(""); } );
    }
    window->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { if (this->on_kill_focus != nullptr) {this->on_kill_focus(""); this->_on_change("");} e.Skip(); });
}

} } // Namespace Slic3r::GUI
