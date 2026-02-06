#include "ThemeManager.hpp"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/sstream.h>
#include "../MainFrame.hpp"
#include <wx/settings.h>
#include <wx/toplevel.h>
#include <wx/notebook.h>
#include <wx/simplebook.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/statbox.h>
#include <wx/radiobut.h>
#include <wx/combobox.h>
#include <wx/choice.h>
#include <wx/listbox.h> // Fixed missing headers
#include <wx/listctrl.h> // Fixed missing headers
#include <wx/treectrl.h> 
#include <functional>

namespace Slic3r { namespace GUI {

bool ThemeManager::m_isDark = true;

void ThemeManager::SetDarkMode(bool dark) {
    if (m_isDark != dark) {
        m_isDark = dark;
        UpdateUI();
    }
}

bool ThemeManager::IsDark() {
    return m_isDark;
}

void ThemeManager::UpdateUI() {
    // Iterate over all top-level windows
    for (auto* win : wxTopLevelWindows) {
        if (!win) continue;
        
        win->Freeze();
        
        // 1. Apply recursive styling to the window and all children
        ApplyThemeRecursive(win);

        // 2. Special MainFrame handling for non-recursive elements (MenuBar, TopBar manual items)
        MainFrame* mainFrame = dynamic_cast<MainFrame*>(win);
        if (mainFrame) {
            mainFrame->sync_colors(); 
        }

        win->Thaw();
        win->Refresh(true);
        win->Update();
    }
}

// Ensure this public static method exists and matches signature in .hpp
void ThemeManager::ApplyThemeRecursive(wxWindow* root) {
     if (!root) return;
     
     // Trigger the update logic on this single root window (and children)
     // We duplicate the logic slightly or refactor. 
     // For minimal risk now, let's copy the logic or call UpdateUI if we can't refactor easily.
     // But ApplyThemeRecursive is intended for ONE window.
     
     ThemeColors theme = GetColors();
     wxColour target_bg = theme.bg;
     
     // 1. Style root itself if it's a container/window
     if (dynamic_cast<wxTopLevelWindow*>(root) || dynamic_cast<wxPanel*>(root)) {
          root->SetBackgroundColour(target_bg);
     }
     
     // 2. Recurse
     // We can just instantiate the same lambda logic here.
      auto should_recurse = [](wxWindow* win) {
        if (dynamic_cast<wxComboBox*>(win) || 
            dynamic_cast<wxChoice*>(win) || 
            dynamic_cast<wxListBox*>(win) || 
            dynamic_cast<wxTextCtrl*>(win) || 
            dynamic_cast<wxCheckBox*>(win) || 
            dynamic_cast<wxRadioButton*>(win)) {
            return false;
        }
        return true;
    };

    std::function<void(wxWindow*)> recurse = [&](wxWindow* parent) {
        if (!parent) return;
        
        wxWindowList& children = parent->GetChildren();
        for (wxWindow* child : children) {
            bool isContainer = dynamic_cast<wxPanel*>(child) || 
                               dynamic_cast<wxNotebook*>(child) || 
                               dynamic_cast<wxSimplebook*>(child) ||
                               dynamic_cast<wxScrolledWindow*>(child);

            if (isContainer) {
                child->SetBackgroundColour(target_bg);
            }

            if (!theme.isDark) {
                if (dynamic_cast<wxTextCtrl*>(child) || dynamic_cast<wxChoice*>(child) || dynamic_cast<wxComboBox*>(child)) {
                    child->SetBackgroundColour(*wxWHITE);
                    child->SetForegroundColour(*wxBLACK);
                }
                else if (dynamic_cast<wxStaticText*>(child) || dynamic_cast<wxCheckBox*>(child) || dynamic_cast<wxRadioButton*>(child) || dynamic_cast<wxStaticBox*>(child)) {
                    child->SetForegroundColour(*wxBLACK);
                }
            } else {
                if (dynamic_cast<wxCheckBox*>(child) || dynamic_cast<wxRadioButton*>(child)) {
                    child->SetForegroundColour(*wxWHITE);
                }
                else if (dynamic_cast<wxStaticText*>(child) || dynamic_cast<wxStaticBox*>(child)) {
                     child->SetForegroundColour(*wxWHITE);
                }
                else if (dynamic_cast<wxTextCtrl*>(child) || dynamic_cast<wxChoice*>(child) || dynamic_cast<wxComboBox*>(child)) {
                    child->SetBackgroundColour(theme.surface);
                    child->SetForegroundColour(theme.text);
                }
                else if (dynamic_cast<wxListBox*>(child) || dynamic_cast<wxListCtrl*>(child) || dynamic_cast<wxTreeCtrl*>(child)) {
                     child->SetBackgroundColour(theme.surface);
                     child->SetForegroundColour(theme.text);
                     child->SetOwnBackgroundColour(theme.surface);
                     child->SetOwnForegroundColour(theme.text);
                }
            }

            if (isContainer || should_recurse(child)) {
                 recurse(child);
            }
            if (child->IsShown()) child->Refresh();
        }
    };
    
    recurse(root);
    if (root->IsShown()) root->Refresh();
}

ThemeColors ThemeManager::GetColors() {
    if (m_isDark) {
        return { 
            wxColour(31, 31, 31),    // bg (Main Content)
            wxColour(45, 45, 45),    // surface (Panels/Inputs)
            wxColour(240, 240, 240), // text
            wxColour(150, 150, 150), // textMuted
            wxColour(117, 117, 12),  // accent
            wxColour(70, 70, 70),    // border
            wxColour(24, 24, 24),    // header (Top Bar)
            true                     // isDark
        };
    }
    return { 
        wxColour(240, 240, 240), // bg
        wxColour(255, 255, 255), // surface
        wxColour(30, 30, 30),    // text
        wxColour(100, 100, 100), // textMuted
        wxColour(0, 90, 180),    // accent
        wxColour(200, 200, 200), // border
        wxColour(230, 230, 230), // header
        false                    // isDark
    };
}

wxBitmapBundle ThemeManager::GetSVG(const wxString& iconName, const wxSize& size, const wxColour& color) {
    // Attempt to locate resources relative to the executable
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName fname(exePath);
    wxString appDir = fname.GetPath();

    // Check standard locations (dev vs installed)
    // Check standard locations (dev vs installed)
    // 1. Next to executable (VS Debug/Release folder)
    wxString resourceBase = appDir + "/resources/images/";
    
    if (!wxDirExists(resourceBase)) {
        // 2. Up one level (Standard specific Slic3r dev layout)
        resourceBase = appDir + "/../resources/images/";
    }
    if (!wxDirExists(resourceBase)) {
        // 3. Current Working Directory fallback
        resourceBase = "resources/images/"; 
    }
    
    wxString fullPath = resourceBase + iconName;
    if (!fullPath.Lower().EndsWith(".svg")) {
        fullPath += ".svg";
    }

    if (!wxFileExists(fullPath)) {
        // Fallback: Try with 'icons' folder just in case
        fullPath = resourceBase + "../icons/" + (m_isDark ? "dark/" : "light/") + iconName + ".svg";
        if (!wxFileExists(fullPath)) return wxBitmapBundle();
    }

    // Read SVG content
    wxFileInputStream input(fullPath);
    if (!input.IsOk()) return wxBitmapBundle();
    
    wxString svgContent;
    wxStringOutputStream output(&svgContent);
    input.Read(output);

    // Recolor if requested
    if (color.IsOk()) {
        wxString hexColor = color.GetAsString(wxC2S_HTML_SYNTAX);
        // Replace standard placeholders or common colors
        // Ideally SVGs should use a specific placeholder like #PLACEHOLDER
        // But for 'tick.svg' it uses #333.
        svgContent.Replace("#333", hexColor); 
        svgContent.Replace("#000000", hexColor);
        svgContent.Replace("#000", hexColor);
        svgContent.Replace("fill=\"black\"", "fill=\"" + hexColor + "\"");
        svgContent.Replace("stroke=\"black\"", "stroke=\"" + hexColor + "\"");
        svgContent.Replace("stroke=\"currentColor\"", "stroke=\"" + hexColor + "\"");
        svgContent.Replace("fill=\"currentColor\"", "fill=\"" + hexColor + "\"");
    }

    return wxBitmapBundle::FromSVG(svgContent.ToStdString().c_str(), size);
}

wxFont ThemeManager::GetFont(FontSize size, FontWeight weight) {
    wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);

    // Size adjustments
    switch (size) {
        case FontSize::Small:
            // Standard UI size
            #ifdef __WXOSX__
            font.SetPointSize(11);
            #endif
            break;
        case FontSize::Medium:
            font.SetPointSize(12);
            break;
        case FontSize::Large:
            font.SetPointSize(14);
            break;
    }

    // Weight adjustments
    if (weight == FontWeight::Bold) {
        font.MakeBold();
    }

    return font;
}

}} // namespace Slic3r::GUI
