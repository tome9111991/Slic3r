#include "OptionsGroup.hpp"
#include "OptionsGroup/Field.hpp"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include "libslic3r/PrintConfig.hpp"
#include "Log.hpp"

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

    auto* line_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    std::string label_text = opt_key;
    if (print_config_def.has(opt_key)) {
        label_text = print_config_def.get(opt_key).label;
    }
    
    auto* label = new wxStaticText(parent, wxID_ANY, label_text + ":");
    line_sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    line_sizer->Add(field->get_window(), 1, wxEXPAND);
    
    if (this->sizer)
        this->sizer->Add(line_sizer, 0, wxEXPAND | wxALL, 5);
}

void OptionsGroup::append_line(const Line& line) {
    auto* line_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    if (!line.label.empty()) {
        auto* label = new wxStaticText(parent, wxID_ANY, line.label + ":");
        line_sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    }
    
    for (const auto& opt : line.options) {
        auto* field = this->build_field(opt.opt_id);
        if (field) {
            _fields[opt.opt_id] = std::unique_ptr<UI_Field>(field);
            line_sizer->Add(field->get_window(), 1, wxEXPAND | wxRIGHT, 5);
        }
    }
    
    if (this->sizer)
        this->sizer->Add(line_sizer, 0, wxEXPAND | wxALL, 5);
}

ConfigOption* OptionsGroup::get_field(const t_config_option_key& opt_key) {
    return nullptr; 
}

void OptionsGroup::update_options(const ConfigBase* config) {
    for (auto& [key, field] : _fields) {
        if (config->has(key)) {
            auto* opt = config->option(key);
            if (!opt) continue;
            
            const ConfigOptionDef* def = nullptr;
            if (print_config_def.has(key)) def = &print_config_def.get(key);
            
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
        }
    }
}

UI_Field* OptionsGroup::build_field(const t_config_option_key& opt_key) {
    ConfigOptionDef def;
    if (print_config_def.has(opt_key)) {
        def = print_config_def.get(opt_key);
    } else {
        def.label = opt_key;
        def.type = coString;
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
            field = f;
            break;
        }
        case coInt:
        case coInts: {
            auto* f = new UI_SpinCtrl(parent, def);
            f->on_change = [this, opt_key](const std::string&, int val) { 
               if (this->on_change) this->on_change(opt_key, val);
            };
            field = f;
            break;
        }
        case coBool:
        case coBools: {
            auto* f = new UI_Checkbox(parent, def);
            f->on_change = [this, opt_key](const std::string&, bool val) { 
               if (this->on_change) this->on_change(opt_key, val);
            };
            field = f;
            break;
        }
        case coEnum: {
            auto* f = new UI_TextCtrl(parent, def);
             f->on_change = [this, opt_key](const std::string&, std::string val) { 
               if (this->on_change) this->on_change(opt_key, val);
            };
            field = f;
            break;
        }
        default: {
            auto* f = new UI_TextCtrl(parent, def);
             f->on_change = [this, opt_key](const std::string&, std::string val) { 
               if (this->on_change) this->on_change(opt_key, val);
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
}

}} // namespace Slic3r::GUI
