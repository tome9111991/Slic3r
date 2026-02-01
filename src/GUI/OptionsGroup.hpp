#include "ConfigBase.hpp"
#include "Config.hpp"
#include "OptionsGroup/Field.hpp"
#include <string>
#include <map>
#include <memory>

namespace Slic3r { namespace GUI {

class Option;
class Line;
class OptionsGroup;
class ConfigOptionsGroup;

/// Class to 
class Option {
public:
    std::string opt_id;
    ConfigOptionDef desc;

    Option(const std::string& id, const ConfigOptionType& _type, const ConfigOption& _def) : opt_id(id) {
        desc.type = _type;
        desc.default_value = _def.clone();
    }

    Option(const Option& other) : opt_id(other.opt_id), desc(other.desc) {
        if (other.desc.default_value)
            desc.default_value = other.desc.default_value->clone();
    }

    Option(Option&& other) noexcept : opt_id(std::move(other.opt_id)), desc(std::move(other.desc)) {
        other.desc.default_value = nullptr;
    }

    Option& operator=(Option other) {
        swap(*this, other);
        return *this;
    }

    ~Option() {
        if (desc.default_value) delete desc.default_value;
        desc.default_value = nullptr;
    }

    friend void swap(Option& first, Option& second) {
        using std::swap;
        swap(first.opt_id, second.opt_id);
        swap(first.desc, second.desc);
    }
};

class Line {
public:
    Line(wxString label, wxString tooltip = "") : label(label), tooltip(tooltip) {}
    
    std::string label;
    std::string tooltip;
    std::vector<Option> options;
    
    void append_option(const Option& opt) {
        options.push_back(opt);
    }
};

class OptionsGroup {
public:
    OptionsGroup(wxWindow* parent) : parent(parent) {}
    
    // Create a new option wrapper for a given key
    Option get_option(const t_config_option_key& opt_key);
    
    // Add a single option as a line
    void append_single_option_line(const t_config_option_key& opt_key);
    
    // Add a constructed line
    void append_line(const Line& line);
    
    // Retrieve the UI field for a given key
    ConfigOption* get_field(const t_config_option_key& opt_id);
    
    UI_Field* get_ui_field(const t_config_option_key& opt_id) {
        auto it = _fields.find(opt_id);
        return (it != _fields.end()) ? it->second.get() : nullptr;
    }

    // Create the widget for a specific option
    UI_Field* build_field(const t_config_option_key& opt_key);

    void set_sizer(wxBoxSizer* s) { sizer = s; }

    void update_options(const ConfigBase* config);

protected:
    wxWindow* parent;
    std::map<t_config_option_key, std::unique_ptr<Option>> _options;
    std::map<t_config_option_key, std::unique_ptr<UI_Field>> _fields;
    wxBoxSizer* sizer; // The sizer this group populates
};

class ConfigOptionsGroup : public OptionsGroup {
public:

    std::shared_ptr<Slic3r::Config> config;
protected:

};

}} // namespace Slic3r::GUI
