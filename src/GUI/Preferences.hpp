#ifndef PREFERENCES_HPP
#define PREFERENCES_HPP

#include <wx/dialog.h>
#include "OptionsGroup.hpp"

namespace Slic3r { namespace GUI {

class PreferencesDialog : public wxDialog {
public:
    PreferencesDialog(wxWindow* parent);

private:
    OptionsGroup* optgroup;
    void _accept();
};

}} // namespace Slic3r::GUI

#endif // PREFERENCES_HPP
