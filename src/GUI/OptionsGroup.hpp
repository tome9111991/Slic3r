#ifndef SLIC3R_GUI_OPTIONSGROUP_HPP
#define SLIC3R_GUI_OPTIONSGROUP_HPP

#include "ConfigBase.hpp"
#include "Config.hpp"
#include "OptionsGroup/Field.hpp"
#include <string>
#include <map>
#include <memory>
#include <wx/gbsizer.h>

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

    // Rely on ConfigOptionDef's copy constructor and destructor
    Option(const Option& other) = default;
    Option(Option&& other) = default;
    Option& operator=(const Option& other) = default;
    Option& operator=(Option&& other) = default;
    ~Option() = default;
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
    OptionsGroup(wxWindow* parent) : parent(parent), show_quick_setting_toggles(true) {}
    
    bool show_quick_setting_toggles {true};
    
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

    void set_sizer(wxBoxSizer* s);

    void update_options(const ConfigBase* config, const std::vector<std::string>& dirty_keys = {});

    std::function<void(const std::string&, boost::any)> on_change {nullptr};
    
    // Callback for Quick Setting Toggle (key, is_active)
    std::function<void(const std::string&, bool)> on_quick_setting_change {nullptr};

    void set_quick_setting_status(const std::string& opt_key, bool active);

protected:
    wxWindow* parent;
    std::map<t_config_option_key, std::unique_ptr<Option>> _options;
    std::map<t_config_option_key, std::unique_ptr<UI_Field>> _fields;
    
    // UI Elements storage
    std::map<t_config_option_key, wxStaticText*> _labels;
    std::map<t_config_option_key, wxStaticText*> _units;
    // We use a simple bitmap button or static bitmap for the star
    std::map<t_config_option_key, wxWindow*> _quick_toggles; 
    
    wxBoxSizer* sizer; // The sizer this group populates
    wxGridBagSizer* grid_sizer {nullptr}; 
    int m_row_count {0};

    wxBitmapBundle star_filled;
    wxBitmapBundle star_empty;
};

class ConfigOptionsGroup : public OptionsGroup {
public:

    std::shared_ptr<Slic3r::Config> config;
protected:

};

}} // namespace Slic3r::GUI

#endif // SLIC3R_GUI_OPTIONSGROUP_HPP
