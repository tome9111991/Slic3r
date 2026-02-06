#include "Theme/ThemeManager.hpp"
#include "misc_ui.hpp"
#include "utils.hpp"
#include "Widgets/ThemedMenu.hpp"
#include <wx/stdpaths.h>
#include <wx/msgdlg.h>
#include <wx/arrstr.h>

#include <exception>
#include <stdexcept>
#include <regex>


namespace Slic3r { namespace GUI {


#ifdef SLIC3R_DEV
void check_version(bool manual)  {
}
#else
void check_version(bool manual)  {
}

#endif

const wxString var(const wxString& in) {
    if (RESOURCE_ABS) {
        return wxString(RESOURCE_ABS_PATH) + "/" + in;
    } else {
        // Robust search logic for resources
        wxString appDir = bin();
        wxArrayString searchPaths;
        
        // 1. Preferred: bin/resources/
        searchPaths.Add(appDir + wxString(RESOURCE_REL) + "/");
        // 2. Dev/VS Layout: bin/../resources/
        searchPaths.Add(appDir + "/.." + wxString(RESOURCE_REL) + "/");
        // 3. Fallback: ./resources/
        searchPaths.Add(wxString(".") + wxString(RESOURCE_REL) + "/");

        for (const auto& path : searchPaths) {
            wxString fullPath = path + in;
            if (wxFileExists(fullPath)) {
                return fullPath;
            }
        }

        // Default fallback if not found
        return appDir + wxString(RESOURCE_REL) + "/" + in;
    }
}

wxBitmapBundle get_bmp_bundle(const wxString& name, int size) {
    wxString base_name = name;
    if (base_name.EndsWith(".png")) base_name.RemoveLast(4);
    if (base_name.EndsWith(".svg")) base_name.RemoveLast(4);
    
    // Determine size if not explicitly handled well by the caller using legacy names
    if (size == 16) { 
        if (base_name.Contains("128px")) size = 128;
        else if (base_name.Contains("192px")) size = 192;
        else if (base_name.Contains("32px")) size = 32;
    }

    wxBitmapBundle svgBundle = ThemeManager::GetSVG(base_name, wxSize(size, size), ThemeManager::GetColors().text);
    if (svgBundle.IsOk()) {
        return svgBundle;
    }

    return wxBitmapBundle();
}

const wxString bin() { 
    wxFileName f(wxStandardPaths::Get().GetExecutablePath());
    wxString appPath(f.GetPath());
    return appPath;
}

const wxString home(const wxString& in) { 
    if (the_os == OS::Windows) 
        return wxGetHomeDir() + "/" + in + "/";
    return wxGetHomeDir() + "/." + in + "/";
}

wxString decode_path(const wxString& in) {
    return in;
}

wxString encode_path(const wxString& in) {
    return in;
}

void show_error(wxWindow* parent, const wxString& message) {
    wxMessageDialog(parent, message, _("Error"), wxOK | wxICON_ERROR).ShowModal();
}

void show_info(wxWindow* parent, const wxString& message, const wxString& title) {
    wxMessageDialog(parent, message, title, wxOK | wxICON_INFORMATION).ShowModal();
}

void fatal_error(wxWindow* parent, const wxString& message) {
    show_error(parent, message);
    throw std::runtime_error(message.ToStdString());
}

wxMenuItem* append_submenu(wxMenu* menu, const wxString& name, const wxString& help, wxMenu* submenu, int id, const wxString& icon) {
    auto* item {new wxMenuItem(menu, id, name, help)};
    
    set_menu_item_icon(item,icon);
    item->SetSubMenu(submenu);
    menu->Append(item);
    return item;
}

ThemedMenu::Item* append_submenu(ThemedMenu* menu, const wxString& name, const wxString& help, ThemedMenu* submenu, int id, const wxString& icon) {
    if (!menu || !submenu) return nullptr;
    return menu->AppendSubMenu(submenu, name, help);
}

void set_menu_item_icon(wxMenuItem* item, const wxString& icon) {
    if (!icon.IsEmpty()) {
        item->SetBitmap(get_bmp_bundle(icon));
    }
}

std::vector<wxString> open_model(wxWindow* parent, wxWindow* top) {
    auto dialog {new wxFileDialog((parent != nullptr ? parent : top), _("Choose one or more files") + wxString(" (STL/OBJ/AMF/3MF):"), ".", "",
    MODEL_WILDCARD, wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST)};
    if (dialog->ShowModal() != wxID_OK) { 
        dialog->Destroy(); 
        return std::vector<wxString>();
    }
    std::vector<wxString> tmp;
    wxArrayString tmpout;
    dialog->GetPaths(tmpout);
    for (const auto& i : tmpout) {
        tmp.push_back(i);
    }
    dialog->Destroy(); 
    return tmp;
}

wxString trim_zeroes(wxString in) { return wxString(_trim_zeroes(in.ToStdString())); }


}} // namespace Slic3r::GUI