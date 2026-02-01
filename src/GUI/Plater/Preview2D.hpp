#ifndef PREVIEW2D_HPP
#define PREVIEW2D_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "PlaterObject.hpp"
#include "Model.hpp"
#include "Config.hpp"
#include "Print.hpp"
#include <vector>
#include <memory>

namespace Slic3r { namespace GUI {

class Preview2DCanvas : public wxPanel {
public:
    Preview2DCanvas(wxWindow* parent);
    void set_print(std::shared_ptr<Slic3r::Print> print);
    void set_z(float z);
    
private:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    
    std::shared_ptr<Slic3r::Print> m_print;
    float m_z = 0.0f;
    
    DECLARE_EVENT_TABLE()
};

class Preview2D : public wxPanel {
public:
    Preview2D(wxWindow* parent, const wxSize& size, std::vector<PlaterObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config);
    void reload_print();
    void enabled(bool enable = true) { m_enabled = enable; }

private:
    void load_print();
    void set_z(float z);

    std::vector<PlaterObject>& objects;
    std::shared_ptr<Slic3r::Model> model;
    std::shared_ptr<Slic3r::Config> config;
    std::shared_ptr<Slic3r::Print> print;
    
    bool m_loaded = false;
    bool m_enabled = false;
    std::vector<float> m_layers_z;
    
    Preview2DCanvas* m_canvas;
    wxSlider* m_slider;
    wxStaticText* m_z_label;
};

} } // Namespace Slic3r::GUI
#endif