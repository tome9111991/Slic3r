#include "Dialogs/PresetEditor.hpp"

namespace Slic3r { namespace GUI {

PrintEditor::PrintEditor(wxWindow* parent, t_config_option_keys options) : 
    PresetEditor(parent, options) {
    
    this->set_tooltips();
    this->config = Slic3r::Config::new_from_defaults(this->my_options());
    this->_update_tree();
    this->load_presets();
    this->_update();
    this->_build();
    }

void PrintEditor::_update(const std::string& opt_key) {
    this->reload_config(); 
    
    // 1. Perimeters
    bool have_perimeters = (config->getInt("perimeters") > 0) || (config->getFloat("min_shell_thickness") > 0);
    const std::vector<std::string> perim_opts = {
        "extra_perimeters", "thin_walls", "overhangs", "seam_position", "external_perimeters_first",
        "external_perimeter_extrusion_width",
        "perimeter_speed", "small_perimeter_speed", "external_perimeter_speed"
    };
    for (const auto& key : perim_opts) {
        if (auto* f = get_ui_field(key)) f->toggle(have_perimeters);
    }
    
    // 2. Adaptive slicing
    bool have_adaptive = config->getBool("adaptive_slicing");
    if (auto* f = get_ui_field("adaptive_slicing_quality")) f->toggle(have_adaptive);
    if (auto* f = get_ui_field("match_horizontal_surfaces")) f->toggle(have_adaptive);
    if (auto* f = get_ui_field("layer_height")) f->toggle(!have_adaptive);
    
    // 3. Sequential printing (complete objects)
    bool complete_objects = config->getBool("complete_objects");
    if (auto* f = get_ui_field("extruder_clearance_radius")) f->toggle(complete_objects);
    if (auto* f = get_ui_field("extruder_clearance_height")) f->toggle(complete_objects);
    
    // 4. Infill
    double density = config->getFloat("fill_density");
    bool have_infill = (density > 0);
    
    const std::vector<std::string> infill_opts = {
        "fill_pattern", "top_infill_pattern", "bottom_infill_pattern",
        "infill_every_layers", "infill_only_where_needed",
        "solid_infill_every_layers", "fill_angle", "solid_infill_below_area",
        "only_retract_when_crossing_perimeters", "infill_first",
        "infill_extruder", "infill_extrusion_width", "infill_overlap",
        "infill_speed", "infill_acceleration"
    };
    for (const auto& key : infill_opts) {
        if (auto* f = get_ui_field(key)) f->toggle(have_infill); 
    }
    
    // 5. Skirt/Brim
    bool have_skirt = (config->getInt("skirts") > 0) || (config->getFloat("min_skirt_length") > 0);
     if (auto* f = get_ui_field("skirt_distance")) f->toggle(have_skirt);
     if (auto* f = get_ui_field("skirt_height")) f->toggle(have_skirt);
     
    // 6. Support material
    bool have_support = config->getBool("support_material");
    const std::vector<std::string> support_opts = {
        "support_material_threshold", "support_material_enforce_layers",
        "support_material_contact_distance", "support_material_pattern",
        "support_material_spacing", "support_material_angle",
        "support_material_interface_layers", "support_material_interface_spacing",
        "support_material_buildplate_only", "dont_support_bridges",
        "support_material_extruder", "support_material_interface_extruder",
        "support_material_extrusion_width", "support_material_interface_extrusion_width",
        "support_material_speed", "support_material_interface_speed"
    };
    for (const auto& key : support_opts) {
        if (auto* f = get_ui_field(key)) f->toggle(have_support);
    }
    
    if (auto* f = get_ui_field("raft_layers")) {
        bool have_raft = (config->getInt("raft_layers") > 0);
        if (have_raft) {
            for (const auto& key : support_opts) {
                if (auto* f2 = get_ui_field(key)) f2->enable();
            }
        }
    }
}


void PrintEditor::_on_preset_loaded() {
    this->_update();
}

void PrintEditor::_build() {
    {
        auto* page = this->add_options_page("Layers and perimeters", "layers.png");
        {
            auto* optgroup = page->new_optgroup("Layer height");
            optgroup->append_single_option_line("layer_height");
            optgroup->append_single_option_line("first_layer_height");
            optgroup->append_single_option_line("adaptive_slicing");
            optgroup->append_single_option_line("adaptive_slicing_quality");
            optgroup->append_single_option_line("match_horizontal_surfaces");
        }
        {
            auto* optgroup = page->new_optgroup("Vertical shells");
            optgroup->append_single_option_line("perimeters");
            optgroup->append_single_option_line("min_shell_thickness");
            optgroup->append_single_option_line("spiral_vase");
        }
        {
            auto* optgroup = page->new_optgroup("Horizontal shells");
            Line line("Solid layers");
            line.append_option(optgroup->get_option("top_solid_layers"));
            line.append_option(optgroup->get_option("bottom_solid_layers"));
            optgroup->append_line(line);
            
            optgroup->append_single_option_line("min_top_bottom_shell_thickness");
        }
        {
            auto* optgroup = page->new_optgroup("Quality (slower slicing)");
            optgroup->append_single_option_line("extra_perimeters");
            optgroup->append_single_option_line("avoid_crossing_perimeters");
            optgroup->append_single_option_line("thin_walls");
            optgroup->append_single_option_line("overhangs");
            optgroup->append_single_option_line("seam_position");
            optgroup->append_single_option_line("external_perimeters_first");
        }
    }
    
    {
        auto* page = this->add_options_page("Infill", "infill.png");
        {
            auto* optgroup = page->new_optgroup("Infill");
            optgroup->append_single_option_line("fill_density");
            optgroup->append_single_option_line("fill_pattern");
            
            Line line("External infill pattern");
            line.append_option(optgroup->get_option("top_infill_pattern"));
            line.append_option(optgroup->get_option("bottom_infill_pattern"));
            optgroup->append_line(line);
        }
        {
            auto* optgroup = page->new_optgroup("Reducing printing time");
            optgroup->append_single_option_line("infill_every_layers");
            optgroup->append_single_option_line("infill_only_where_needed");
        }
        {
            auto* optgroup = page->new_optgroup("Advanced");
            optgroup->append_single_option_line("fill_gaps");
            optgroup->append_single_option_line("solid_infill_every_layers");
            optgroup->append_single_option_line("fill_angle");
            optgroup->append_single_option_line("solid_infill_below_area");
            optgroup->append_single_option_line("only_retract_when_crossing_perimeters");
            optgroup->append_single_option_line("infill_first");
        }
    }
    
    {
        auto* page = this->add_options_page("Skirt and brim", "box.png");
        {
            auto* optgroup = page->new_optgroup("Skirt");
            optgroup->append_single_option_line("skirts");
            optgroup->append_single_option_line("skirt_distance");
            optgroup->append_single_option_line("skirt_height");
            optgroup->append_single_option_line("min_skirt_length");
        }
        {
            auto* optgroup = page->new_optgroup("Brim");
            optgroup->append_single_option_line("brim_width");
            optgroup->append_single_option_line("brim_ears");
            optgroup->append_single_option_line("brim_ears_max_angle");
            optgroup->append_single_option_line("interior_brim_width");
            optgroup->append_single_option_line("brim_connections_width");
        }
    }

    {
        auto* page = this->add_options_page("Support material", "building.png");
        {
            auto* optgroup = page->new_optgroup("Support material");
            optgroup->append_single_option_line("support_material");
            optgroup->append_single_option_line("support_material_threshold");
            optgroup->append_single_option_line("support_material_max_layers");
            optgroup->append_single_option_line("support_material_enforce_layers");
        }
        {
            auto* optgroup = page->new_optgroup("Raft");
            optgroup->append_single_option_line("raft_layers");
        }
        {
            auto* optgroup = page->new_optgroup("Options for support material and raft");
            optgroup->append_single_option_line("support_material_contact_distance");
            optgroup->append_single_option_line("support_material_pattern");
            optgroup->append_single_option_line("support_material_spacing");
            optgroup->append_single_option_line("support_material_angle");
            optgroup->append_single_option_line("support_material_pillar_size");
            optgroup->append_single_option_line("support_material_pillar_spacing");
            optgroup->append_single_option_line("support_material_interface_layers");
            optgroup->append_single_option_line("support_material_interface_spacing");
            optgroup->append_single_option_line("support_material_buildplate_only");
            optgroup->append_single_option_line("dont_support_bridges");
        }
    }

    {
        auto* page = this->add_options_page("Speed", "time.png");
        {
            auto* optgroup = page->new_optgroup("Speed for print moves");
            optgroup->append_single_option_line("perimeter_speed");
            optgroup->append_single_option_line("small_perimeter_speed");
            optgroup->append_single_option_line("external_perimeter_speed");
            optgroup->append_single_option_line("infill_speed");
            optgroup->append_single_option_line("solid_infill_speed");
            optgroup->append_single_option_line("top_solid_infill_speed");
            optgroup->append_single_option_line("gap_fill_speed");
            optgroup->append_single_option_line("bridge_speed");
            optgroup->append_single_option_line("support_material_speed");
            optgroup->append_single_option_line("support_material_interface_speed");
        }
        {
            auto* optgroup = page->new_optgroup("Speed for non-print moves");
            optgroup->append_single_option_line("travel_speed");
        }
        {
            auto* optgroup = page->new_optgroup("Modifiers");
            optgroup->append_single_option_line("first_layer_speed");
        }
        {
            auto* optgroup = page->new_optgroup("Acceleration control (advanced)");
            optgroup->append_single_option_line("perimeter_acceleration");
            optgroup->append_single_option_line("infill_acceleration");
            optgroup->append_single_option_line("bridge_acceleration");
            optgroup->append_single_option_line("first_layer_acceleration");
            optgroup->append_single_option_line("default_acceleration");
        }
        {
            auto* optgroup = page->new_optgroup("Autospeed (advanced)");
            optgroup->append_single_option_line("max_print_speed");
            optgroup->append_single_option_line("max_volumetric_speed");
        }
    }

    {
        auto* page = this->add_options_page("Multiple extruders", "funnel.png");
        {
            auto* optgroup = page->new_optgroup("Extruders");
            optgroup->append_single_option_line("perimeter_extruder");
            optgroup->append_single_option_line("infill_extruder");
            optgroup->append_single_option_line("solid_infill_extruder");
            optgroup->append_single_option_line("support_material_extruder");
            optgroup->append_single_option_line("support_material_interface_extruder");
        }
        {
            auto* optgroup = page->new_optgroup("Ooze prevention");
            optgroup->append_single_option_line("ooze_prevention");
            optgroup->append_single_option_line("standby_temperature_delta");
        }
        {
            auto* optgroup = page->new_optgroup("Advanced");
            optgroup->append_single_option_line("regions_overlap");
            optgroup->append_single_option_line("interface_shells");
        }
    }

    {
        auto* page = this->add_options_page("Advanced", "wand.png");
        {
            auto* optgroup = page->new_optgroup("Extrusion width");
            optgroup->append_single_option_line("extrusion_width");
            optgroup->append_single_option_line("first_layer_extrusion_width");
            optgroup->append_single_option_line("perimeter_extrusion_width");
            optgroup->append_single_option_line("external_perimeter_extrusion_width");
            optgroup->append_single_option_line("infill_extrusion_width");
            optgroup->append_single_option_line("solid_infill_extrusion_width");
            optgroup->append_single_option_line("top_infill_extrusion_width");
            optgroup->append_single_option_line("support_material_interface_extrusion_width");
            optgroup->append_single_option_line("support_material_extrusion_width");
        }
        {
            auto* optgroup = page->new_optgroup("Overlap");
            optgroup->append_single_option_line("infill_overlap");
        }
        {
            auto* optgroup = page->new_optgroup("Flow");
            optgroup->append_single_option_line("bridge_flow_ratio");
        }
        {
            auto* optgroup = page->new_optgroup("Other");
            optgroup->append_single_option_line("xy_size_compensation");
            optgroup->append_single_option_line("resolution");
        }
    }

    {
        auto* page = this->add_options_page("Output options", "page_white_go.png");
        {
            auto* optgroup = page->new_optgroup("Sequential printing");
            optgroup->append_single_option_line("complete_objects");
            
            Line line("Extruder clearance (mm)");
            line.append_option(optgroup->get_option("extruder_clearance_radius"));
            line.append_option(optgroup->get_option("extruder_clearance_height"));
            optgroup->append_line(line);
        }
        {
            auto* optgroup = page->new_optgroup("Output file");
            optgroup->append_single_option_line("gcode_comments");
            optgroup->append_single_option_line("label_printed_objects");
            optgroup->append_single_option_line("output_filename_format");
        }
        {
            auto* optgroup = page->new_optgroup("Post-processing scripts");
            optgroup->append_single_option_line("post_process");
        }
    }

    {
        auto* page = this->add_options_page("Notes", "note.png");
        {
            auto* optgroup = page->new_optgroup("Notes");
            optgroup->append_single_option_line("notes");
        }
    }
}
}} // namespace Slic3r::GUI
