#ifndef PRESET_CHOOSER_HPP
#define PRESET_CHOOSER_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
    #include <wx/panel.h>
    #include <wx/sizer.h>
#endif
#include <wx/bmpcbox.h>
#include "../Widgets/ThemedControls.hpp"
#include "../Widgets/PresetWidget.hpp"

#include <vector>
#include <array>
#include <functional>

#include "Print.hpp"

#include "misc_ui.hpp"
#include "Preset.hpp"
#include "GUI.hpp"
#include "Settings.hpp"

namespace Slic3r { namespace GUI {

using Choosers = std::vector<ThemedSelect*>;
using chooser_name_list = std::vector<wxString>;
using chooser_name_map = std::array<chooser_name_list, preset_types>;

class PresetChooser : public wxPanel {
public:
    std::function<void(preset_t)> on_change {nullptr};

    /// Build a panel to contain a sizer for dropdowns for preset selection.
    PresetChooser(wxWindow* parent, std::weak_ptr<Print> print);
    PresetChooser(wxWindow* parent, std::weak_ptr<Print> print, Settings* external_settings, preset_store& external_presets);

    std::array<std::vector<PresetSection*>, preset_types> preset_sections;


    /// Load the presets from the backing store and set up the choosers
    void load(std::array<Presets, preset_types> presets);

    wxSize DoGetBestSize() const override;
    
    /// Call load() with the app's own presets
    void load() { this->load(this->_presets); }

    /// Const reference to internal name map (used for testing)
    const chooser_name_map& _chooser_names() const { return this->__chooser_names; }

    /// Set the selection of one of the preset lists to the entry matching the
    /// supplied name.
    /// \param[in]  name    Name of preset to select. Case-sensitive.
    /// \param[in]  group   Type of preset to change.
    /// \param[in]  index   Preset chooser index to operate on (default is 0)
    /// \return Whether or not the preset was actually updated.
    ///
    /// Note: If index is greater than the number of active presets, nothing
    /// happens.
    /// Note: If name is not found, nothing happens.
    bool select_preset_by_name(wxString name, preset_t group, size_t index);

    /// Set the selection of one of the preset lists to the entry matching the
    /// supplied name.
    /// \param[in]  name    Name of preset to select. Case-sensitive.
    /// \param[in]  chooser Direct pointer to the appropriate wxBitmapComboBox
    /// \return Whether or not the preset was actually updated.
    ///
    /// Note: If name is not found, nothing happens.
    bool select_preset_by_name(wxString name, ThemedSelect* chooser);

    /// Cycle through active presets and prompt user to save dirty configs, if necessary.
    bool prompt_unsaved_changes();

    /// Fetch the preset name corresponding to the chooser index
    wxString _get_selected_preset(preset_t group, size_t index) const;

private:
    wxBoxSizer* _local_sizer {};
    wxWindow* _parent {};
    void _on_change_combobox(preset_t preset, size_t index, const wxString& selection);
    chooser_name_map __chooser_names;

    /// Reference to a Slic3r::Settings object.
    Settings* _settings;

    /// Weak reference to Plater's print
    std::weak_ptr<Print> _print;

    /// Reference to owning Application's preset database.
    preset_store& _presets;

    void _on_select_preset(preset_t preset);

    /// Return the vector of strings representing the selected preset names.
    std::vector<wxString> _get_selected_presets(preset_t group) const;

    /// Update Settings presets with the state of this system
    void _update_preset_settings(preset_t preset);

public:
    /// Update UI colors and icons for the current theme
    void UpdateTheme();

private:
    std::vector<wxStaticText*> m_labels;
    std::vector<ThemedButton*> m_settings_buttons;
};

}} // Slic3r::GUI
#endif // PRESET_CHOOSER_HPP
