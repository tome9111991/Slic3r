#ifndef PREVIEW3D_HPP
#define PREVIEW3D_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "PlaterObject.hpp"
#include "Scene3D.hpp"
#include "Model.hpp"
#include "Config.hpp"
#include "Print.hpp"

namespace Slic3r { namespace GUI {

class PreviewScene3D : public Scene3D {
public:
    PreviewScene3D(wxWindow* parent, const wxSize& size) : Scene3D(parent,size){}

    void load_print_toolpaths(std::shared_ptr<Slic3r::Print> print);
    void set_toolpaths_range(float min_z, float max_z);
    
    // Expose set_bed_shape
    using Scene3D::set_bed_shape;
    
    void resetObjects(){
        volumes.clear();
        layers.clear();
    }
    
protected:
    virtual void before_render() override;
    virtual void after_render() override;

private:
   struct LayerData {
       float z;
       std::vector<float> verts;
       std::vector<unsigned char> colors;
   };
   std::vector<LayerData> layers;
   float m_max_z = std::numeric_limits<float>::max();
};

class Preview3D : public wxPanel {
public:
    void reload_print();
    void load_print();
    void set_bed_shape(const std::vector<Point>& shape);
    Preview3D(wxWindow* parent, const wxSize& size, std::shared_ptr<Slic3r::Print> _print, std::vector<PlaterObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config);
    void enabled(bool enable = true) {}
    
    PreviewScene3D* get_canvas() { return &canvas; }

private:
    void set_z(float z);
    bool loaded = false, _enabled = false;
    std::vector<float> layers_z;
    std::shared_ptr<Slic3r::Print> print;
    PreviewScene3D canvas;
    wxSlider* slider;
    wxStaticText* z_label;
    std::vector<PlaterObject>& objects; //< reference to parent vector
    std::shared_ptr<Slic3r::Model> model;
    std::shared_ptr<Slic3r::Config> config;
};

} } // Namespace Slic3r::GUI
#endif
