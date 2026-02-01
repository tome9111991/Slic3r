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
}

void MaterialEditor::_build() {
}

void MaterialEditor::_on_preset_loaded() {
    this->_update();
}
}} // namespace Slic3r::GUI