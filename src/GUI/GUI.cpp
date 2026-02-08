#include <wx/display.h>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
    #include <wx/filefn.h> 
    #include <wx/dir.h>
#endif
#include <wx/display.h>
#include <wx/stopwatch.h>
#include <string>

#include "libslic3r.h"
#include "MainFrame.hpp"
#include "GUI.hpp"
#include "misc_ui.hpp"
#include "Settings.hpp"
#include "Preset.hpp"
#include "ConfigWizard.hpp"
#include "Widgets/SplashScreen.hpp"

// Logging mechanism
#include "Log.hpp"
#include "Theme/ThemeManager.hpp"

namespace Slic3r { namespace GUI {

/// Primary initialization and point of entry into the GUI application.
/// Calls MainFrame and handles preset loading, etc.
bool App::OnInit()
{
    try {
        this->SetAppName("Slic3r");
        
        // Initialize image handlers early to load splash screen
        wxImage::AddHandler(new wxPNGHandler());

        if (datadir.empty())
            datadir = decode_path(wxStandardPaths::Get().GetUserDataDir());

        // Initialize global paths
        set_data_dir(datadir.ToStdString());
        
        // Locate resources directory
        wxString exe_path = wxStandardPaths::Get().GetExecutablePath();
        wxFileName exe_fn(exe_path);
        wxString res_path = exe_fn.GetPath() + "/resources";
        if (!wxDirExists(res_path)) res_path = exe_fn.GetPath() + "/../resources";
        if (!wxDirExists(res_path)) res_path = exe_fn.GetPath() + "/../../resources";
        // Dev fallback
        if (!wxDirExists(res_path)) res_path = "c:/Users/Tommi/Documents/Codeing/Slic3r-master/resources";
        
        set_resources_dir(res_path.ToStdString());
        set_var_dir(res_path.ToStdString()); // var_dir often overlaps with resources in simple setups
        set_local_dir(res_path.ToStdString() + "/localization");
        
        Slic3r::Log::info(LogChannel, "Resources Dir: " + resources_dir());

        // Initialize custom fonts from resources
        ThemeManager::InitFonts();

        ui_settings = Settings::init_settings();
        ui_settings->load_settings();

        SplashScreen* splash = new SplashScreen("Slic3r", SLIC3R_VERSION);
        splash->Show();
        wxStopWatch sw; // Start timing here
        
        splash->SetStatus(_("Loading configuration..."));

        this->notifier = std::unique_ptr<Notifier>();
        wxString enc_datadir = encode_path(datadir);
        
        const wxString& slic3r_ini  {datadir + "/slic3r.ini"};
        this->preset_ini[static_cast<int>(preset_t::Print)] = {datadir + "/print"};
        this->preset_ini[static_cast<int>(preset_t::Printer)] = {datadir + "/printer"};
        this->preset_ini[static_cast<int>(preset_t::Material)] = {datadir + "/filament"};
        const wxString& print_ini = this->preset_ini[static_cast<int>(preset_t::Print)];
        const wxString& printer_ini = this->preset_ini[static_cast<int>(preset_t::Printer)];
        const wxString& material_ini = this->preset_ini[static_cast<int>(preset_t::Material)];


        // if we don't have a datadir or a slic3r.ini, prompt for wizard.
        bool run_wizard = (!wxDirExists(datadir) || !wxFileExists(slic3r_ini));
        
        if (run_wizard) {
             // Destroy splash screen before showing wizard
             if (splash) {
                 splash->Destroy();
                 splash = nullptr;
             }

             // Ensure datadir exists at least
             if (!wxDirExists(datadir)) wxMkdir(datadir);
             
             ConfigWizard wizard(nullptr);
             if (wizard.run()) {
                 // Save initial config
                 // wizard.config.save(slic3r_ini); // TODO: Properly save wizard result to presets
             }
        }
        
        /* Check to make sure if datadir exists */
        for (auto& dir : std::vector<wxString> { enc_datadir, print_ini, printer_ini, material_ini }) {
            if (wxDirExists(dir)) continue;
            if (!wxMkdir(dir)) {
                Slic3r::Log::fatal_error(LogChannel, (_("Slic3r was unable to create its data directory at ")+ dir).ToStdWstring());
            }
        }

        Slic3r::Log::info(LogChannel, (_("Data dir: ") + datadir).ToStdWstring());

        // Load presets
        splash->SetStatus(_("Loading presets..."));
        this->load_presets();
        ui_settings->save_settings();

        // Create the main frame while the splash screen is still visible
        // This utilizes the waiting time for heavy UI initialization
        splash->SetStatus(_("Initializing UI..."));
        MainFrame *frame = new MainFrame( "Slic3r", wxDefaultPosition, wxDefaultSize);
        
        // Layout early so canvases get their correct sizes for GL initialization
        frame->Layout();

        // Pre-initialize 3D Engine (Shaders, ImGui, GLAD) while splash is showing
        if (frame->get_plater()) {
            splash->SetStatus(_("Initializing 3D Engine..."));
            auto init_canvas = [splash](Scene3D* canvas, const wxString& name) {
                if (canvas) {
                    splash->SetStatus(_("Initializing ") + name + "...");
                    // SetCurrent requires the window to be at least created (which MainFrame is)
                    canvas->SetCurrent(*canvas->GetContext());
                    canvas->init_gl();
                }
            };
            
            init_canvas(frame->get_plater()->get_canvas3d(), "3D Editor");
            if (frame->get_plater()->get_preview3d()) {
                init_canvas(frame->get_plater()->get_preview3d()->get_canvas(), "G-code Preview");
            }
        }

        // Ensure splash is visible for at least 2000ms
        // We use a loop with wxSafeYield to keep the splash screen responsive 
        // without blocking the entire UI thread or showing the main frame.
        while (sw.Time() < 2000) {
            wxSafeYield(); 
            wxMilliSleep(50);
        }

        // Now that we've waited and the frame is ready, show it and remove splash
        if (splash) {
            splash->Destroy();
            splash = nullptr;
        }
        
        this->SetTopWindow(frame);
        frame->Show(true);

        // run callback functions during idle on the main frame
        this->Bind(wxEVT_IDLE, 
            [this](wxIdleEvent& e) { 
                std::function<void()> func {nullptr}; // 
                // try to get the mutex. If we can't, just skip this idle event and get the next one.
                if (!this->callback_register.try_lock()) return;
                // pop callback
                if (cb.size() >= 1) { 
                    func = cb.top();
                    cb.pop();
                }
                // unlock mutex
                this->callback_register.unlock();
                try { // call the function if it's not nullptr;
                    if (func != nullptr) func();
                } catch (std::exception& e) { Slic3r::Log::error(LogChannel, LOG_WSTRING("Exception thrown: " <<  e.what())); }
            });

        return true;
    } catch (const std::exception& e) {
        Slic3r::Log::fatal_error(LogChannel, LOG_WSTRING("Exception in OnInit: " << e.what()));
        return false;
    } catch (...) {
        Slic3r::Log::fatal_error(LogChannel, L"Unknown exception in OnInit");
        return false;
    }
}

#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

void App::MSWForceDarkMode(bool use_dark_mode) 
{
    // 1. Force Title Bars of all current windows
    HMODULE hDwmapi = LoadLibraryA("dwmapi.dll");
    if (hDwmapi) {
        typedef HRESULT(WINAPI * t_DwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
        t_DwmSetWindowAttribute pDwmSetWindowAttribute = (t_DwmSetWindowAttribute)GetProcAddress(hDwmapi, "DwmSetWindowAttribute");
        
        if (pDwmSetWindowAttribute) {
            BOOL value = use_dark_mode ? TRUE : FALSE;
            
            // Apply to all top-level windows (MainFrame, Dialogs, etc.)
            for (wxWindow* win : wxTopLevelWindows) {
                if (win && win->GetHWND()) {
                    pDwmSetWindowAttribute((HWND)win->GetHWND(), DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
                    
                    // Force redraw of the non-client area (Title bar)
                    ::SetWindowPos((HWND)win->GetHWND(), NULL, 0, 0, 0, 0, 
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                }
            }
        }
        FreeLibrary(hDwmapi);
    }

    // 2. Hint common controls (Explorer, Open/Save Dialogs) via undocumented uxtheme API
    // Ordinal 135 is SetPreferredAppMode (Windows 10 1903+)
    HMODULE hUxtheme = LoadLibraryA("uxtheme.dll");
    if (hUxtheme) {
        typedef void (WINAPI * t_SetPreferredAppMode)(int);
        t_SetPreferredAppMode pSetPreferredAppMode = (t_SetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
        
        if (pSetPreferredAppMode) {
            // 0: Default, 1: AllowDark, 2: ForceDark, 3: ForceMax
            // We use 2 for ForceDark if active, else 0 (Default/Light)
            int mode = use_dark_mode ? 2 : 0;
            pSetPreferredAppMode(mode);
            
            // Also need to flush the theme change? 
            // Usually DwmSetWindowAttribute + SetWindowPos triggers enough of a repaint.
        }
        FreeLibrary(hUxtheme);
    }
}
#endif

void App::save_window_pos(const wxTopLevelWindow* window, const wxString& name ) {
    ui_settings->window_pos[name] = 
        std::make_tuple<wxPoint, wxSize, bool>(
            window->GetScreenPosition(),
            window->GetSize(),
            window->IsMaximized());

    ui_settings->save_settings();
}

void App::restore_window_pos(wxTopLevelWindow* window, const wxString& name ) {
    try {
        auto tmp = ui_settings->window_pos[name];
        const auto& size = std::get<1>(tmp);
        const auto& pos = std::get<0>(tmp);
        window->SetSize(size);

        auto display = wxDisplay().GetClientArea();
        if (((pos.x + size.x / 2) < display.GetRight()) && (pos.y + size.y/2 < display.GetBottom()))
            window->Move(pos);

        window->Maximize(std::get<2>(tmp));
    }
    catch (std::out_of_range& /*e*/) {
        // config was empty
    }
}

void App::load_presets() {
    for (size_t group = 0; group < preset_types; ++group) {
        Presets& preset_list = this->presets.at(group);
        wxString& ini = this->preset_ini.at(group);
        // keep external or dirty presets (remove those that are NOT external/dirty)
        preset_list.erase(std::remove_if(preset_list.begin(), preset_list.end(), 
                    [](const Preset& t) -> bool { return !((t.external && t.file_exists()) || t.dirty()); }),
                preset_list.end());
        if (wxDirExists(ini)) {
            auto sink { wxDirTraverserSimple() };
            sink.file_cb = ([&preset_list, group] (const wxString& filename) {

                    // skip if we already have it
                    if (std::find_if(preset_list.begin(), preset_list.end(), 
                            [filename] (const Preset& t) 
                                { return filename.ToStdString() == t.name; }) != preset_list.end()) return;
                    wxString path, name, ext;
                    wxFileName::SplitPath(filename, &path, &name, &ext);

                    preset_list.push_back(Preset(path.ToStdString(), (name + wxString(".") + ext).ToStdString(), static_cast<preset_t>(group)));
                    });

            wxDir dir(ini);
            dir.Traverse(sink, "*.ini");

            // Sort the list by name
            std::sort(preset_list.begin(), preset_list.end(), [] (const Preset& x, const Preset& y) -> bool { return x.name < y.name; });

            // Prepend default Preset
            preset_list.emplace(preset_list.begin(), Preset(true, "- default -"s, static_cast<preset_t>(group)));
        }

    }
}

void App::CallAfter(std::function<void()> cb_function) {
    // set mutex
    this->callback_register.lock();
    // push function onto stack
    this->cb.emplace(cb_function);
    // unset mutex
    this->callback_register.unlock();
}

void App::OnUnhandledException() {
    try { throw; } 
    catch (std::exception &e) {
        Slic3r::Log::fatal_error(LogChannel, LOG_WSTRING("Exception Caught: " << e.what()));
    }
}

}} // namespace Slic3r::GUI