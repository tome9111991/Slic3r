#include "OptionsGroup.hpp"
#include "OptionsGroup/Field.hpp"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include "libslic3r/PrintConfig.hpp"

namespace Slic3r { namespace GUI {

Option OptionsGroup::get_option(const t_config_option_key& opt_key) {
    // Use the comprehensive PrintConfig definition
    if (PrintConfig::def->has(opt_key)) {
        const ConfigOptionDef& def = PrintConfig::def->get(opt_key);
        return Option(opt_key, def.type, *def.default_value);
    }
    // Fallback
    ConfigOptionDef def;
    def.label = opt_key;
    // Dummy default
    ConfigOptionString dummy(""); 
    return Option(opt_key, coString, dummy);
}

void OptionsGroup::append_single_option_line(const t_config_option_key& opt_key) {
    auto* field = this->build_field(opt_key);
    if (!field) return;
    
    // Store the field
    _fields[opt_key] = std::unique_ptr<UI_Field>(field);

    auto* line_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Get label from definition
    std::string label_text = opt_key;
    if (PrintConfig::def->has(opt_key)) {
        label_text = PrintConfig::def->get(opt_key).label;
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
            // This is tricky because UI_Field::set_value takes boost::any
            // We need to get value from config as right type.
            // ConfigBase has optptr/opt methods.
            
            // Allow basic types for now.
            // We can use ConfigOption::serialize/deserialize?
            // Or dynamic_cast the option and get value.
            
            // Better: Let's use get_option(key) to get type, then use config-implementation
            
            // Simplification: Try to get string/int/bool/float properties
            // Ideally UI_Field should know how to read from ConfigOption.
            
            auto* opt = config->option(key);
            if (!opt) continue;
            
            const ConfigOptionDef* def = nullptr;
            if (PrintConfig::def->has(key)) def = &PrintConfig::def->get(key);
            
            if (def) {
                 try {
                     switch (def->type) {
                         case coBool: field->set_value(opt->getBool()); break;
                         case coInt: field->set_value(opt->getInt()); break;
                         case coFloat: field->set_value(opt->getFloat()); break;
                         case coString: field->set_value(opt->getString()); break;
                        case coEnum: {
                              // UI_Choice needs index or string?
                              // usually string for keys.
                              // ConfigOptionEnumGeneric or specialized.
                              // Let's rely on serialize() if get_string() fails?
                              // ConfigOption has getString() which throws if not string.
                               // ConfigOptionEnum overrides serialize not getString.
                               field->set_value(opt->serialize());
                               break;
                        }
                         default: 
                             // Try string serialization
                             field->set_value(opt->serialize());
                             break;
                     }
                 } catch (...) {
                     // ignore mismatch
                 }
            }
        }
    }
}

UI_Field* OptionsGroup::build_field(const t_config_option_key& opt_key) {
    ConfigOptionDef def;
    if (PrintConfig::def->has(opt_key)) {
        def = PrintConfig::def->get(opt_key);
    } else {
        def.label = opt_key;
        def.type = coString;
    }
    
    switch (def.type) {
        case coFloat:
        case coFloats:
        case coString:
            return new UI_TextCtrl(parent, def);
        case coInt:
        case coInts:
            return new UI_SpinCtrl(parent, def);
        case coBool:
        case coBools:
            return new UI_Checkbox(parent, def);
        case coEnum:
            return new UI_Choice(parent, def);
        default:
            return new UI_TextCtrl(parent, def);
    }
}

}}
