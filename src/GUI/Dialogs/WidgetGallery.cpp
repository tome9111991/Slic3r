#include "WidgetGallery.hpp"
#include "../Widgets/ThemedControls.hpp"
#include "../Theme/ThemeManager.hpp"
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace Slic3r { namespace GUI {

WidgetGallery::WidgetGallery(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Theme Widget Gallery", wxDefaultPosition, wxSize(600, 500), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    InitGui();
    CenterOnParent();
    
    // Ensure background follows theme
    SetBackgroundColour(ThemeManager::GetColors().bg);
    Bind(wxEVT_PAINT, [this](wxPaintEvent&){
         wxPaintDC dc(this);
         dc.SetBackground(wxBrush(ThemeManager::GetColors().bg));
         dc.Clear();
    });
    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) { this->wxWindow::SetFocus(); e.Skip(); });
}

void WidgetGallery::InitGui()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    auto theme = ThemeManager::GetColors();

    auto addSection = [&](const wxString& title, wxSizer* parent) {
        wxStaticText* lbl = new wxStaticText(this, wxID_ANY, title);
        lbl->SetFont(ThemeManager::GetFont(ThemeManager::FontSize::Medium, ThemeManager::FontWeight::Bold));
        lbl->SetForegroundColour(theme.text);
        parent->Add(lbl, 0, wxALL, 10);
    };

    // 1. Buttons
    addSection("Buttons", mainSizer);
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    
    ThemedButton* btn1 = new ThemedButton(this, wxID_ANY, "Primary Button");
    ThemedButton* btn2 = new ThemedButton(this, wxID_ANY, "Secondary");
    
    btnSizer->Add(btn1, 0, wxRIGHT, 10);
    btnSizer->Add(btn2, 0, wxRIGHT, 10);
    mainSizer->Add(btnSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 20);

    // 2. Checkboxes
    addSection("Native Replacements", mainSizer);
    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 10, 20);
    
    // Checkbox
    ThemedCheckBox* chk1 = new ThemedCheckBox(this, wxID_ANY, "Themed Checkbox");
    ThemedCheckBox* chk2 = new ThemedCheckBox(this, wxID_ANY, "Checked by default");
    chk2->SetValue(true);
    
    grid->Add(chk1);
    grid->Add(chk2);
    
    // Select / Dropdown
    wxArrayString opts; opts.Add("Option A"); opts.Add("Option B"); opts.Add("Option C");
    ThemedSelect* sel = new ThemedSelect(this, wxID_ANY, opts);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Themed Select:")); // Label logic missing in simple add, but ok for demo
    grid->Add(sel);

    mainSizer->Add(grid, 0, wxLEFT | wxRIGHT | wxBOTTOM, 20);

    // 3. Inputs
    addSection("Inputs", mainSizer);
    
    wxFlexGridSizer* inputGrid = new wxFlexGridSizer(2, 5, 20);
    
    // Text Input
    ThemedTextInput* txt = new ThemedTextInput(this, wxID_ANY, "Some Text");
    inputGrid->Add(new wxStaticText(this, wxID_ANY, "Text Input:")); 
    inputGrid->Add(txt);
    
    // Number Input
    ThemedNumberInput* num = new ThemedNumberInput(this, wxID_ANY, 210.0);
    num->SetRange(0, 300);
    
    inputGrid->Add(new wxStaticText(this, wxID_ANY, "Temperature (°C):")); 
    inputGrid->Add(num);
    
    mainSizer->Add(inputGrid, 0, wxLEFT | wxRIGHT | wxBOTTOM, 20);

    // Close Button
    wxButton* closeBtn = new wxButton(this, wxID_OK, "Close");
    mainSizer->Add(closeBtn, 0, wxALIGN_CENTER | wxALL, 10);

    SetSizer(mainSizer);
    Layout();
}

void WidgetGallery::OnClose(wxCommandEvent& evt)
{
    EndModal(wxID_OK);
}

}} // namespace Slic3r::GUI
