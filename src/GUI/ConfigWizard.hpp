#ifndef CONFIGWIZARD_HPP
#define CONFIGWIZARD_HPP

#include <wx/wizard.h>
#include <wx/wx.h>
#include <wx/stattext.h>
#include "libslic3r/Config.hpp"
#include <vector>

namespace Slic3r { namespace GUI {

class ConfigWizardPage : public wxWizardPageSimple {
public:
    ConfigWizardPage(wxWizard* parent, const wxString& title, const wxString& short_title = "");
    virtual ~ConfigWizardPage() {}
    
    // Helper to get logic configuration from the wizard
    DynamicPrintConfig* config();
    
    // Helpers to add UI elements
    void append_text(wxString text);
    // Stub for appending option input (would use OptionsGroup in full impl)
    // For now we might use simple widgets for the MVP
    
protected:
    wxBoxSizer* vsizer;
    wxString short_title;
    friend class ConfigWizard;
};

class ConfigWizard : public wxWizard {
public:
    ConfigWizard(wxWindow* parent);
    ~ConfigWizard();
    
    bool run();
    
    DynamicPrintConfig config; // The accumulator config
    
private:
    std::vector<ConfigWizardPage*> pages;
    
    void add_page(ConfigWizardPage* page);
};

}} // namespace Slic3r::GUI

#endif // CONFIGWIZARD_HPP
