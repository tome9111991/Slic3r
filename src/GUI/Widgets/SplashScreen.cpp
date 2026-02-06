#include "SplashScreen.hpp"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/display.h>
#include "../misc_ui.hpp"

namespace Slic3r { namespace GUI {

SplashScreen::SplashScreen(const wxString& title, const wxString& version)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(500, 500), 
              wxFRAME_NO_TASKBAR | wxBORDER_NONE | wxSTAY_ON_TOP | wxFRAME_SHAPED),
      m_version(version), m_status("Initializing...")
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(ThemeManager::GetColors().surface);
    
    // Load Logo (PNG)
    wxString logoPath = var("images/slic3r_scale.png");
    if (wxFileExists(logoPath)) {
        wxBitmap bmp;
        if (bmp.LoadFile(logoPath, wxBITMAP_TYPE_PNG)) {
            m_logo = wxBitmapBundle::FromBitmap(bmp);
        }
    }
    
    // Fallback if not found
    if (!m_logo.IsOk()) {
        m_logo = ThemeManager::GetSVG("Slic3r_192px_transparent", wxSize(160, 160));
    }
    
    // Center on screen
    wxDisplay display(wxDisplay::GetFromWindow(this));
    wxRect screen = display.GetClientArea();
    SetPosition(wxPoint(screen.x + (screen.width - 500) / 2, 
                         screen.y + (screen.height - 500) / 2));

    // Fix for Windows: Mask the window shape to match the rounded corners
    // This prevents the "grey" box from appearing behind the rounded edges.
    wxGraphicsPath maskPath = wxGraphicsContext::Create()->CreatePath();
    maskPath.AddRoundedRectangle(0, 0, 500, 500, 12);
    SetShape(maskPath);
    
    Bind(wxEVT_PAINT, &SplashScreen::OnPaint, this);
}

void SplashScreen::SetStatus(const wxString& status)
{
    m_status = status;
    Refresh();
    Update();
}

void SplashScreen::OnPaint(wxPaintEvent& evt)
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();

    auto colors = ThemeManager::GetColors();

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;

    // 1. Draw Background (Rounded Rect)
    gc->SetBrush(wxBrush(colors.surface));
    gc->SetPen(wxPen(colors.border, 1));
    gc->DrawRoundedRectangle(0, 0, 500, 500, 12);

    // 2. Draw Logo
    wxBitmap bmp = m_logo.GetBitmap(wxSize(160, 160));
    if (bmp.IsOk()) {
        gc->DrawBitmap(bmp, (500 - 160) / 2, 80, 160, 160);
    }

    // 3. Draw App Name
    gc->SetFont(ThemeManager::GetFont(ThemeManager::FontSize::Large, ThemeManager::FontWeight::Bold), colors.text);
    wxDouble tw, th, td, tel;
    gc->GetTextExtent("Slic3r", &tw, &th, &td, &tel);
    gc->DrawText("Slic3r", (500 - tw) / 2, 260);

    // 4. Draw Version
    gc->SetFont(ThemeManager::GetFont(ThemeManager::FontSize::Medium, ThemeManager::FontWeight::Normal), colors.textMuted);
    gc->GetTextExtent(m_version, &tw, &th, &td, &tel);
    gc->DrawText(m_version, (500 - tw) / 2, 310);

    // 5. Draw Status
    gc->SetFont(ThemeManager::GetFont(ThemeManager::FontSize::Small, ThemeManager::FontWeight::Normal), colors.textMuted);
    gc->GetTextExtent(m_status, &tw, &th, &td, &tel);
    gc->DrawText(m_status, (500 - tw) / 2, 380);

    // 6. Draw Footer
    wxString footer = "The original open-source slicer project";
    gc->SetFont(ThemeManager::GetFont(ThemeManager::FontSize::Small, ThemeManager::FontWeight::Normal), colors.textMuted.ChangeLightness(80));
    gc->GetTextExtent(footer, &tw, &th, &td, &tel);
    gc->DrawText(footer, (500 - tw) / 2, 460);
}

}} // namespace Slic3r::GUI
