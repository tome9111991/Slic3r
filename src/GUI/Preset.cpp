#include "Preset.hpp"
#include "Config.hpp"
#include <regex>
#include <algorithm>
#include <wx/msgdlg.h>
#include "Log.hpp"

#include "Dialogs/PresetEditor.hpp"
#include "GUI.hpp"

using namespace std::literals::string_literals;
using namespace boost;

namespace Slic3r { namespace GUI {

Preset::Preset(bool is_default, wxString name, preset_t p) : group(p), name(name), external(false), default_preset(is_default) {
    t_config_option_keys keylist;
    switch (this->group) {
        case preset_t::Print:
            keylist = PrintEditor::options();
            break;
        case preset_t::Material:
            keylist = MaterialEditor::options();
            break;
        case preset_t::Printer:
            keylist = PrinterEditor::options();
            break;
        default: break;
    }
    this->_dirty_config = Slic3r::Config::new_from_defaults(keylist);
    this->_config = Slic3r::Config::new_from_defaults(keylist);
}

Preset::Preset(std::string load_dir, std::string filename, preset_t p) : group(p), _file(wxFileName(load_dir, filename)) {
    this->name = this->_file.GetName();
    this->_dirty_config = Slic3r::Config::new_from_ini(_file.GetFullPath().ToStdString());
    this->_config = Slic3r::Config::new_from_ini(_file.GetFullPath().ToStdString());

}

void Preset::save() {
    if (this->default_preset) {
        Slic3r::Log::error("GUI", "Attempted to save default preset.");
        return;
    }
    
    if (!this->_file.IsOk() || this->name.empty()) {
        Slic3r::Log::error("GUI", "Attempted to save preset without filename.");
        return;
    }

    try {
        // Write to a temporary file first to avoid locking issues (read/write conflict)
        std::string target_path = this->_file.GetFullPath().ToStdString();
        std::string temp_path = target_path + ".tmp";
        
        // Save to temp file
        this->_dirty_config->config().save(temp_path);
        
        // Safe replace logic
        if (wxFileExists(target_path)) {
            if (!wxRemoveFile(target_path)) {
                 Slic3r::Log::error("GUI", "Failed to remove existing preset file before update: " + target_path);
                 // Fallback: Try to move anyway (OS might handle it if not locked)
            }
        }
        
        if (!wxRenameFile(temp_path, target_path)) {
            // If rename fails, try copy and delete
             if (!wxCopyFile(temp_path, target_path)) {
                  throw std::runtime_error("Failed to atomic move/copy preset file.");
             }
             wxRemoveFile(temp_path);
        }

        // Reload the config from disk to ensure consistency
        // This verifies the save meant what we thought it meant, and handles FP truncation etc.
        config_ptr loaded_cfg = Slic3r::Config::new_from_ini(target_path);
        if (loaded_cfg && !loaded_cfg->empty()) {
            this->_config = loaded_cfg;
            // Overwrite dirty config with the loaded file content so they are identical
            this->_dirty_config->apply(this->_config);
        } else {
            // Fallback
             this->_config->apply(this->_dirty_config);
        }
        
        Slic3r::Log::info("GUI", "Saved preset to " + target_path);
    } catch (std::exception& e) {
        wxMessageBox(wxString::Format("Error saving preset: %s", e.what()), "Error", wxICON_ERROR);
    }
}

// Stubs to satisfy linker / header
bool Preset::save_as(wxString new_name, t_config_option_keys opt_keys) {
    if (new_name.empty()) return false;

    // Determine directory
    wxString dir;
    if (_file.IsOk() && _file.DirExists()) {
        dir = _file.GetPath();
    } else {
        // Use global app datadir - requires GUI.hpp
        if (SLIC3RAPP) {
             wxString datadir = SLIC3RAPP->datadir;
             switch (this->group) {
                case preset_t::Print: dir = datadir + "/print"; break;
                // Note: 'filament' is the legacy folder name for Material
                case preset_t::Material: dir = datadir + "/filament"; break; 
                case preset_t::Printer: dir = datadir + "/printer"; break;
                default: return false;
            }
        } else {
            return false;
        }
    }
    
    if (!wxDirExists(dir)) {
        if (!wxMkdir(dir)) {
             Slic3r::Log::error("GUI", "Could not create directory for preset: " + dir.ToStdString());
             return false;
        }
    }

    wxFileName new_file(dir, new_name, "ini");
    
    // Save
    try {
        // If opt_keys is provided, we might want to filter, but for now we save the whole dirty config
        // which mimics standard Slic3r behavior for "Save As". 
        this->_dirty_config->config().save(new_file.GetFullPath().ToStdString());
        
        // Update self to point to new file
        this->name = new_name;
        this->_file = new_file;
        this->default_preset = false;
        this->external = false;
        
        // Sync config
        this->_config->apply(this->_dirty_config);
        
        Slic3r::Log::info("GUI", "Saved preset as " + new_file.GetFullPath().ToStdString());
        return true;
    } catch (std::exception& e) {
        wxMessageBox(wxString::Format("Error saving preset: %s", e.what()), "Error", wxICON_ERROR);
        return false;
    }
}

void Preset::delete_preset() {
    if (this->default_preset) {
        wxMessageBox(_("You cannot delete a default preset."), _("Error"), wxICON_ERROR);
        return;
    }
    
    if (this->_file.FileExists()) {
        if (wxRemoveFile(this->_file.GetFullPath())) {
            Slic3r::Log::info("GUI", "Deleted preset: " + this->_file.GetFullPath().ToStdString());
        } else {
            Slic3r::Log::error("GUI", "Failed to delete preset: " + this->_file.GetFullPath().ToStdString());
        }
    }
}
bool Preset::prompt_unsaved_changes(wxWindow* parent) { return false; }
void Preset::dismiss_changes() {}


t_config_option_keys Preset::dirty_options() const {
    t_config_option_keys dirty;

    auto diff_config = this->_config->diff(this->_dirty_config);
    std::move(diff_config.begin(), diff_config.end(), std::back_inserter(dirty));

    auto extra = this->_group_overrides();
    std::copy_if(extra.cbegin(), extra.cend(), std::back_inserter(dirty), 
            [this](const std::string x) { return !this->_config->has(x) && this->_dirty_config->has(x);});

    dirty.erase(std::remove_if(dirty.begin(), dirty.end(), 
            [this](const std::string x) { return this->_config->has(x) && !this->_dirty_config->has(x);}),
            dirty.end());

    return dirty;
}

bool Preset::dirty() const { 
    return this->dirty_options().size() > 0;
}

Slic3r::Config Preset::dirty_config() {
    if (!this->loaded()) load_config();
    Slic3r::Config result { Slic3r::Config(*(this->_dirty_config)) };
    return result;
}

config_ref Preset::config() {
    std::weak_ptr<Slic3r::Config> result { this->_dirty_config };
    return result;
}

config_ptr Preset::load_config() {
    if (this->loaded()) return this->_dirty_config;

    t_config_option_keys keys { this->_group_keys() };
    t_config_option_keys extra_keys { this->_group_overrides() };

    if (this->default_preset) {
        this->_config = Slic3r::Config::new_from_defaults(keys);
    } else if (this->_file.HasName()) {
        config_ptr config = Slic3r::Config::new_from_defaults(keys);
        if (this->file_exists()) {
            config_ptr external_config = Slic3r::Config::new_from_ini(this->_file.GetFullPath().ToStdString());
            // Apply preset values on top of defaults
            config = Slic3r::Config::new_from_defaults(keys);
            config->apply_with_defaults(external_config, keys);

            // For extra_keys don't populate defaults.
            if (extra_keys.size() > 0 && !this->external) {
                config->apply(external_config, extra_keys);
            }

            this->_config = config;
        }
    }

    this->_dirty_config->apply(this->_config);
    return this->_dirty_config;
}

t_config_option_keys Preset::_group_keys() const {
    switch (this->group) {
        case preset_t::Print:
            return PrintEditor::options();
        case preset_t::Material:
            return MaterialEditor::options();
        case preset_t::Printer:
            return PrinterEditor::options();
        default:
            return t_config_option_keys();
    }
}
t_config_option_keys Preset::_group_overrides() const {
    switch (this->group) {
        case preset_t::Print:
            return PrintEditor::overriding_options();
        case preset_t::Material:
            return MaterialEditor::overriding_options();
        case preset_t::Printer:
            return PrinterEditor::overriding_options();
        default:
            return t_config_option_keys();
    }
}

bool Preset::compatible(const std::string& printer_name) const {
    if (!this->_dirty_config->has("compatible_printers") || this->default_preset || this->group == preset_t::Printer) {
        return true;
    }
    auto compatible_list {this->_dirty_config->get<ConfigOptionStrings>("compatible_printers").values};
    if (compatible_list.size() == 0) return true;
    return std::any_of(compatible_list.cbegin(), compatible_list.cend(), [printer_name] (const std::string& x) -> bool { return x.compare(printer_name) == 0; });
}

const std::string preset_name(preset_t group) {
    switch(group) {
        case preset_t::Print:
            return "Print"s;
        case preset_t::Printer:
            return "Printer"s;
        case preset_t::Material:
            return "Material"s;
        default:
            return "N/A"s;
    }
}


}} // namespace Slic3r::GUI
