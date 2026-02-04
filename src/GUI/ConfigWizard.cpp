#include "ConfigWizard.hpp"
#include "GUI.hpp"
#include "libslic3r/PrintConfig.hpp"

namespace Slic3r { namespace GUI {

ConfigWizardPage::ConfigWizardPage(wxWizard* parent, const wxString& title, const wxString& short_title)
    : wxWizardPageSimple(parent)
{
    this->short_title = short_title.IsEmpty() ? title : short_title;
    
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(main_sizer);
    
    // Title
    wxStaticText* title_txt = new wxStaticText(this, wxID_ANY, title);
    // Use ThemeManager for consistent Large font
    wxFont font = ThemeManager::GetFont(ThemeManager::FontSize::Large);
    font.SetWeight(wxFONTWEIGHT_BOLD);
    title_txt->SetFont(font);
    
    main_sizer->Add(title_txt, 0, wxALL, 10);
    
    // Content area
    this->vsizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(this->vsizer, 1, wxEXPAND | wxALL, 10);
}

DynamicPrintConfig* ConfigWizardPage::config() {
    return &((static_cast<ConfigWizard*>(GetParent()))->config);
}

void ConfigWizardPage::append_text(wxString text) {
    wxStaticText* txt = new wxStaticText(this, wxID_ANY, text);
    txt->Wrap(400); 
    this->vsizer->Add(txt, 0, wxBOTTOM, 10);
}


ConfigWizard::ConfigWizard(wxWindow* parent) 
    : wxWizard(parent, wxID_ANY, _("Configuration Wizard"))
{
    this->SetPageSize(wxSize(500, 400));
}

ConfigWizard::~ConfigWizard() {
}

void ConfigWizard::add_page(ConfigWizardPage* page) {
    if (!pages.empty()) {
        wxWizardPageSimple::Chain(pages.back(), page);
    }
    pages.push_back(page);
    this->GetPageAreaSizer()->Add(page);
}

bool ConfigWizard::run() {
    // 1. Welcome Page
    auto p1 = new ConfigWizardPage(this, _("Welcome to Slic3r Configuration Assistant"), "Welcome");
    p1->append_text(_("Hello, welcome to Slic3r! This assistant helps you with the initial configuration; just a few settings and you will be ready to print."));
    this->add_page(p1);
    
    // 2. Firmware (Simplified for MVP)
    auto p2 = new ConfigWizardPage(this, _("Firmware Type"), "Firmware");
    p2->append_text(_("Choose the type of firmware used by your printer."));
    // TODO: Add dropdown for gcode_flavor
    this->add_page(p2);

    // 3. Bed Size
    auto p3 = new ConfigWizardPage(this, _("Bed Size"), "Bed");
    p3->append_text(_("Set the shape of your printer's bed."));
    // TODO: Add BedShapePanel logic
    this->add_page(p3);

    // 4. Finished
    auto pEnd = new ConfigWizardPage(this, _("Congratulations!"), "Finish");
    pEnd->append_text(_("You have successfully completed the Slic3r Configuration Assistant."));
    this->add_page(pEnd);
    
    if (this->RunWizard(pages[0])) {
        // Apply logic
        return true;
    }
    return false;
}

}} // namespace Slic3r::GUI
