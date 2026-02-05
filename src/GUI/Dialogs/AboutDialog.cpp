#include "Dialogs/AboutDialog.hpp"
#include "Theme/ThemeManager.hpp"
#include <wx/dcclient.h>
#include <wx/stattext.h>
#include <wx/button.h>

namespace Slic3r { namespace GUI {

static void link_clicked(wxHtmlLinkEvent& e)
{
    wxLaunchDefaultBrowser(e.GetLinkInfo().GetHref());
    e.Skip(false);
}

// ----------------------------------------------------------------------------
// AboutDialogLogo Implementation
// ----------------------------------------------------------------------------

AboutDialogLogo::AboutDialogLogo(wxWindow* parent) : 
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize) 
{ 
    // User requested to keep using PNG for the logo for now
    this->logo = wxBitmap(var("images/Slic3r_192px.png"), wxBITMAP_TYPE_PNG);
    if (this->logo.IsOk()) {
        this->SetMinSize(wxSize(this->logo.GetWidth(), this->logo.GetHeight()));
    } else {
        this->SetMinSize(wxSize(192, 192)); // Fallback size
    }

    this->Bind(wxEVT_PAINT, [this](wxPaintEvent& e) { this->repaint(e); });
}

void AboutDialogLogo::repaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);

    dc.SetBackgroundMode(wxPENSTYLE_TRANSPARENT);

    if (!this->logo.IsOk()) {
        event.Skip();
        return;
    }

    const wxSize size = this->GetSize();
    const int logo_w = this->logo.GetWidth();
    const int logo_h = this->logo.GetHeight();

    dc.DrawBitmap(this->logo, (size.GetWidth() - logo_w) / 2, (size.GetHeight() - logo_h) / 2, true);
    // event.Skip(); // Don't skip if we handled painting, usually
}

// ----------------------------------------------------------------------------
// AboutDialog Implementation
// ----------------------------------------------------------------------------

AboutDialog::AboutDialog(wxWindow* parent) 
    : wxDialog(parent, wxID_ANY, _("About Slic3r"), wxDefaultPosition, wxSize(600, 460), wxDEFAULT_DIALOG_STYLE)
{ 
    auto* hsizer = new wxBoxSizer(wxHORIZONTAL);
    auto* vsizer = new wxBoxSizer(wxVERTICAL);

    // logo
    auto* logo = new AboutDialogLogo(this);
    hsizer->Add(logo, 0, wxEXPAND | wxLEFT | wxRIGHT, 30);

    // title
    auto* title = new wxStaticText(this, wxID_ANY, "Slic3r", wxDefaultPosition, wxDefaultSize);
    
    // Title is very specific (Roman, 24pt)
    wxFont title_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    title_font.SetWeight(wxFONTWEIGHT_BOLD);
    title_font.SetFamily(wxFONTFAMILY_ROMAN);
    title_font.SetPointSize(24);
    title->SetFont(title_font);

    vsizer->Add(title, 0, wxALIGN_LEFT | wxTOP, 30);

    // version
    // SLIC3R_VERSION is a string literal "1.5.0"
    wxString version_str = "Version " + wxString(SLIC3R_VERSION);
    auto* version = new wxStaticText(this, wxID_ANY, version_str, wxDefaultPosition, wxDefaultSize);
    
    // Use ThemeManager for standard UI font
    auto version_font = ThemeManager::GetFont(ThemeManager::FontSize::Small);
    version->SetFont(version_font);
    vsizer->Add(version, 0, wxALIGN_LEFT | wxBOTTOM, 10);

    // text
    wxString text;
    text << "<html>"
         << "<body>"
         << "Copyright &copy; 2011-2017 Alessandro Ranellucci. <br />" 
         << "<a href=\"https://slic3r.org/\">Slic3r</a> is licensed under the "
         << "<a href=\"https://www.gnu.org/licenses/agpl-3.0.html\">GNU Affero General Public License, version 3</a>."
         << "<br /><br /><br />"
         << "Contributions by Henrik Brix Andersen, Vojtech Bubnik, Nicolas Dandrimont, Mark Hindess, "
         << "Petr Ledvina, Joseph Lenox, Y. Sapir, Mike Sheldrake, Kliment Yanev and numerous others. "
         << "Manual by Gary Hodgson. Inspired by the RepRap community. <br />"
         << "Slic3r logo designed by Corey Daniels, <a href=\"http://www.famfamfam.com/lab/icons/silk/\">Silk Icon Set</a> designed by Mark James. "
         << "<br /><br />"
         << "Built on " << build_date << " at git version " << git_version << "."
         << "</body>"
         << "</html>";

    auto* html = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_NEVER);
    html->SetBorders(2);
    html->SetPage(text);

    html->Bind(wxEVT_HTML_LINK_CLICKED, [](wxHtmlLinkEvent& e){ link_clicked(e); });

    vsizer->Add(html, 1, wxEXPAND | wxRIGHT | wxBOTTOM, 20);

    // buttons
    auto buttons = this->CreateStdDialogButtonSizer(wxOK);
    this->SetEscapeId(wxID_CLOSE);
    
    // Check if buttons were actually created (sometimes NULL if flags invalid)
    if (buttons) {
        vsizer->Add(buttons, 0, wxEXPAND | wxRIGHT | wxBOTTOM, 3);
    }

    hsizer->Add(vsizer, 1, wxEXPAND, 0);
    this->SetSizer(hsizer);
    this->Layout();
    this->Centre();
}

}} // namespace Slic3r::GUI
