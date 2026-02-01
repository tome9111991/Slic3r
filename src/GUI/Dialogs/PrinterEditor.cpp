#include "Dialogs/PresetEditor.hpp"
#include "libslic3r/PrintConfig.hpp"

namespace Slic3r { namespace GUI {

PrinterEditor::PrinterEditor(wxWindow* parent, t_config_option_keys options) : 
    PresetEditor(parent, options) {
    
    this->set_tooltips();
    this->config = Slic3r::Config::new_from_defaults(this->my_options());
    this->_update_tree();
    this->load_presets();
    this->_update();
    this->_build();
    }

void PrinterEditor::_update(const std::string& opt_key) {
    this->reload_config();
    
    // Logic for toggling fields
    bool use_firmware_retraction = config->getBool("use_firmware_retraction");
    
    // Safely access retract_length (vector)
    bool have_retract_length = false;
    if (auto* opt = config->get_ptr<Slic3r::ConfigOptionFloats>("retract_length")) {
        have_retract_length = (!opt->values.empty() && opt->values[0] > 0);
    }

    bool retraction = (have_retract_length || use_firmware_retraction);

    if (auto* f = get_ui_field("retract_length")) f->toggle(!use_firmware_retraction);
    
    if (auto* f = get_ui_field("retract_before_travel")) f->toggle(retraction);
    
    if (auto* f = get_ui_field("retract_lift")) f->toggle(retraction && !use_firmware_retraction);
    if (auto* f = get_ui_field("retract_layer_change")) f->toggle(retraction);
    
    // Safely access retract_lift (vector)
    bool have_retract_lift = false;
    if (auto* opt = config->get_ptr<Slic3r::ConfigOptionFloats>("retract_lift")) {
        have_retract_lift = (!opt->values.empty() && opt->values[0] > 0);
    }
    
    if (auto* f = get_ui_field("retract_lift_above")) f->toggle(retraction && have_retract_lift);
    if (auto* f = get_ui_field("retract_lift_below")) f->toggle(retraction && have_retract_lift);
    
    if (auto* f = get_ui_field("retract_restart_extra")) f->toggle(retraction && !use_firmware_retraction);
    if (auto* f = get_ui_field("wipe")) f->toggle(retraction && !use_firmware_retraction);
    
    if (auto* f = get_ui_field("retract_speed")) f->toggle(retraction);

    // Host type logic
    bool has_print_host = !config->getString("print_host").empty();
    if (auto* f = get_ui_field("octoprint_apikey")) f->toggle(has_print_host);
    
    // Multiple extruders
    bool have_multiple_extruders = false;
    if (auto* opt = config->get_ptr<Slic3r::ConfigOptionFloats>("nozzle_diameter")) {
        have_multiple_extruders = (opt->values.size() > 1);
    }
    if (auto* f = get_ui_field("toolchange_gcode")) f->toggle(have_multiple_extruders);
}

void PrinterEditor::_build() {
    {
        auto* page = this->add_options_page("General", "printer_empty.png");
        {
            auto* optgroup = page->new_optgroup("Size and coordinates");
            // Bed shape button missing in C++ framework, skipped.
            optgroup->append_single_option_line("z_offset");
        }
        {
            auto* optgroup = page->new_optgroup("Capabilities");
            optgroup->append_single_option_line("extruders_count");
            optgroup->append_single_option_line("has_heatbed");
        }
        {
            auto* optgroup = page->new_optgroup("USB/Serial connection");
            optgroup->append_single_option_line("serial_port");
            optgroup->append_single_option_line("serial_speed");
        }
        {
            auto* optgroup = page->new_optgroup("Print server upload");
            optgroup->append_single_option_line("host_type");
            optgroup->append_single_option_line("print_host");
            optgroup->append_single_option_line("octoprint_apikey");
        }
        {
            auto* optgroup = page->new_optgroup("Firmware");
            optgroup->append_single_option_line("gcode_flavor");
        }
        {
            auto* optgroup = page->new_optgroup("Advanced");
            optgroup->append_single_option_line("use_relative_e_distances");
            optgroup->append_single_option_line("use_firmware_retraction");
            optgroup->append_single_option_line("use_volumetric_e");
            optgroup->append_single_option_line("pressure_advance");
            optgroup->append_single_option_line("vibration_limit");
            optgroup->append_single_option_line("z_steps_per_mm");
            optgroup->append_single_option_line("use_set_and_wait_extruder");
            optgroup->append_single_option_line("use_set_and_wait_bed");
            optgroup->append_single_option_line("fan_percentage");
        }
    }

    {
        auto* page = this->add_options_page("Custom G-code", "script.png");
        {
            auto* optgroup = page->new_optgroup("Start G-code");
            optgroup->append_single_option_line("start_gcode");
        }
        {
            auto* optgroup = page->new_optgroup("End G-code");
            optgroup->append_single_option_line("end_gcode");
        }
        {
            auto* optgroup = page->new_optgroup("Before layer change G-code");
            optgroup->append_single_option_line("before_layer_gcode");
        }
        {
            auto* optgroup = page->new_optgroup("After layer change G-code");
            optgroup->append_single_option_line("layer_gcode");
        }
        {
            auto* optgroup = page->new_optgroup("Tool change G-code");
            optgroup->append_single_option_line("toolchange_gcode");
        }
        {
            auto* optgroup = page->new_optgroup("Between objects G-code (for sequential printing)");
            optgroup->append_single_option_line("between_objects_gcode");
        }
    }
    
    // Extruder 1 (Static for now)
    {
        auto* page = this->add_options_page("Extruder 1", "funnel.png");
        {
            auto* optgroup = page->new_optgroup("Size");
            optgroup->append_single_option_line("nozzle_diameter");
        }
        {
            auto* optgroup = page->new_optgroup("Layer Height Limits");
            optgroup->append_single_option_line("min_layer_height");
            optgroup->append_single_option_line("max_layer_height");
        }
        {
            auto* optgroup = page->new_optgroup("Position (for multi-extruder printers)");
            optgroup->append_single_option_line("extruder_offset");
        }
        {
            auto* optgroup = page->new_optgroup("Retraction");
            optgroup->append_single_option_line("retract_length");
            optgroup->append_single_option_line("retract_lift");
            
            Line line("Only lift Z");
            line.append_option(optgroup->get_option("retract_lift_above"));
            line.append_option(optgroup->get_option("retract_lift_below"));
            optgroup->append_line(line);
            
            optgroup->append_single_option_line("retract_speed");
            optgroup->append_single_option_line("retract_restart_extra");
            optgroup->append_single_option_line("retract_before_travel");
            optgroup->append_single_option_line("retract_layer_change");
            optgroup->append_single_option_line("wipe");
        }
        {
            auto* optgroup = page->new_optgroup("Retraction when tool is disabled (advanced settings for multi-extruder setups)");
            optgroup->append_single_option_line("retract_length_toolchange");
            optgroup->append_single_option_line("retract_restart_extra_toolchange");
        }
    }

    {
        auto* page = this->add_options_page("Notes", "note.png");
        {
            auto* optgroup = page->new_optgroup("Notes");
            optgroup->append_single_option_line("printer_notes");
        }
    }
}

void PrinterEditor::_on_preset_loaded() {
    this->_update();
}
}} // namespace Slic3r::GUI