#include "Dialogs/PresetEditor.hpp"
namespace Slic3r { namespace GUI {

MaterialEditor::MaterialEditor(wxWindow* parent, t_config_option_keys options) : 
    PresetEditor(parent, options) {
    
    this->set_tooltips();
    this->config = Slic3r::Config::new_from_defaults(this->my_options());
    this->_update_tree();
    this->load_presets();
    this->_update();
    this->_build();
    }

void MaterialEditor::_update(const std::string& opt_key) {
    this->reload_config();
    
    bool cooling = config->getBool("cooling");
    bool fan_always_on = config->getBool("fan_always_on");
    
    if (auto* f = get_ui_field("max_fan_speed")) f->toggle(cooling);
    if (auto* f = get_ui_field("fan_below_layer_time")) f->toggle(cooling);
    if (auto* f = get_ui_field("slowdown_below_layer_time")) f->toggle(cooling);
    if (auto* f = get_ui_field("min_print_speed")) f->toggle(cooling);
    
    if (auto* f = get_ui_field("min_fan_speed")) f->toggle(cooling || fan_always_on);
    if (auto* f = get_ui_field("disable_fan_first_layers")) f->toggle(cooling || fan_always_on);
}

void MaterialEditor::_build() {
    {
        auto* page = this->add_options_page("Filament", "spool.png");
        {
            auto* optgroup = page->new_optgroup("Filament");
            optgroup->append_single_option_line("filament_colour");
            optgroup->append_single_option_line("filament_diameter");
            optgroup->append_single_option_line("extrusion_multiplier");
        }
        {
            auto* optgroup = page->new_optgroup("Temperature (°C)");
            Line line("Extruder");
            line.append_option(optgroup->get_option("first_layer_temperature"));
            line.append_option(optgroup->get_option("temperature"));
            optgroup->append_line(line);
            
            Line line2("Bed");
            line2.append_option(optgroup->get_option("first_layer_bed_temperature"));
            line2.append_option(optgroup->get_option("bed_temperature"));
            optgroup->append_line(line2);
        }
        {
            auto* optgroup = page->new_optgroup("Optional information");
            optgroup->append_single_option_line("filament_density");
            optgroup->append_single_option_line("filament_cost");
        }
    }
    
    {
        auto* page = this->add_options_page("Cooling", "hourglass.png");
        {
            auto* optgroup = page->new_optgroup("Enable");
            optgroup->append_single_option_line("fan_always_on");
            optgroup->append_single_option_line("cooling");
        }
        {
            auto* optgroup = page->new_optgroup("Fan settings");
            Line line("Fan speed");
            line.append_option(optgroup->get_option("min_fan_speed"));
            line.append_option(optgroup->get_option("max_fan_speed"));
            optgroup->append_line(line);
            
            optgroup->append_single_option_line("bridge_fan_speed");
            optgroup->append_single_option_line("disable_fan_first_layers");
        }
        {
            auto* optgroup = page->new_optgroup("Cooling thresholds");
            optgroup->append_single_option_line("fan_below_layer_time");
            optgroup->append_single_option_line("slowdown_below_layer_time");
            optgroup->append_single_option_line("min_print_speed");
        }
    }
    
    {
        auto* page = this->add_options_page("Custom G-code", "script.png");
        {
            auto* optgroup = page->new_optgroup("Start G-code");
            optgroup->append_single_option_line("start_filament_gcode");
        }
        {
            auto* optgroup = page->new_optgroup("End G-code");
            optgroup->append_single_option_line("end_filament_gcode");
        }
    }
    
    {
        auto* page = this->add_options_page("Notes", "note.png");
        {
            auto* optgroup = page->new_optgroup("Notes");
            optgroup->append_single_option_line("filament_notes");
        }
    }
    
    {
        auto* page = this->add_options_page("Overrides", "wrench.png");
        {
            auto* optgroup = page->new_optgroup("Overrides");
            optgroup->append_single_option_line("filament_max_volumetric_speed");
        }
    }
}

void MaterialEditor::_on_preset_loaded() {
    this->_update();
}
}} // namespace Slic3r::GUI