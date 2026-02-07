#include "Preview3D.hpp"
#include <wx/event.h>
#include "libslic3r.h"
#include "ExtrusionEntity.hpp"
#include "Layer.hpp"
#include "ExtrusionGeometry.hpp"
#include "ExtrusionEntityCollection.hpp"
#include "Theme/CanvasTheme.hpp"

namespace Slic3r { namespace GUI {

void PreviewScene3D::load_print_toolpaths(std::shared_ptr<Slic3r::Print> print) {
    this->volumes.clear();
    this->layers.clear();
    
    if(!print || print->objects.empty()) return;

    CanvasThemeColors theme = CanvasTheme::GetColors();

    struct VolData {
        std::vector<Slic3r::ExtrusionGeometry::InstanceData> instances;
    };
    std::map<unsigned long, VolData> vol_map; // Key: RGB value

    // Helper to process extrusion collection recursively
    std::function<void(const ExtrusionEntityCollection&, const Point&, double, std::map<unsigned long, VolData>&)> process_collection;
    process_collection = [&](const ExtrusionEntityCollection& collection, const Point& copy, double top_z, std::map<unsigned long, VolData>& vol_map) {
        for(const auto* entity : collection.entities) {
            Lines lines;
            std::vector<double> widths;
            std::vector<double> heights;
            ExtrusionRole role = erNone;

            if (const auto* path = dynamic_cast<const ExtrusionPath*>(entity)) {
                Polyline polyline = path->polyline;
                polyline.remove_duplicate_points();
                // Translation handled by extrusion_to_instanced
                lines = polyline.lines();
                
                double w = ::Slic3r::ExtrusionGeometry::get_stadium_width(path->mm3_per_mm, path->height, path->width);
                widths.assign(lines.size(), w);
                heights.assign(lines.size(), path->height);
                role = path->role;
            } else if (const auto* loop = dynamic_cast<const ExtrusionLoop*>(entity)) {
                for(const auto& path : loop->paths) {
                    Polyline polyline = path.polyline;
                    polyline.remove_duplicate_points();
                    // Translation handled by extrusion_to_instanced
                    Lines path_lines = polyline.lines();
                    lines.insert(lines.end(), path_lines.begin(), path_lines.end());
                    
                    double w = ::Slic3r::ExtrusionGeometry::get_stadium_width(path.mm3_per_mm, path.height, path.width);
                    widths.insert(widths.end(), path_lines.size(), w);
                    heights.insert(heights.end(), path_lines.size(), path.height);
                }
                if (!loop->paths.empty()) role = loop->paths.front().role;
            } else if (const auto* sub = dynamic_cast<const ExtrusionEntityCollection*>(entity)) {
                process_collection(*sub, copy, top_z, vol_map);
                continue;
            }

            if(!lines.empty() && role != erNone) {
                wxColor c = theme.get_role_color(role);
                unsigned long color_key = c.GetRGB();
                float gl_color[4] = { c.Red()/255.0f, c.Green()/255.0f, c.Blue()/255.0f, 1.0f };
                
                ::Slic3r::ExtrusionGeometry::extrusion_to_instanced(lines, widths, heights, top_z, copy, gl_color, vol_map[color_key].instances);
            }
        }
    };

    for (auto* object : print->objects) {
        for(const auto& copy : object->_shifted_copies) {
            for (auto* layer : object->layers) {
                for (auto* region : layer->regions) {
                    process_collection(region->perimeters, copy, layer->print_z, vol_map);
                    process_collection(region->fills, copy, layer->print_z, vol_map);
                }
            }
            for (auto* layer : object->support_layers) {
                process_collection(layer->support_fills, copy, layer->print_z, vol_map);
                process_collection(layer->support_interface_fills, copy, layer->print_z, vol_map);
            }
        }
    }

    // Finalize volumes
    for(auto& pair : vol_map) {
        Volume vol;
        vol.color.SetRGB(pair.first);
        vol.origin = Pointf3(0,0,0);
        vol.instances = std::move(pair.second.instances);
        
        // Sort instances by Z height (PosA.z) to allow drawing only up to specific layer
        std::sort(vol.instances.begin(), vol.instances.end(), [](const Slic3r::ExtrusionGeometry::InstanceData& a, const Slic3r::ExtrusionGeometry::InstanceData& b) {
            return a.posA[2] < b.posA[2];
        });

        vol.is_instanced = true;
        vol.gpu_dirty = true;
        
        // Compute BB roughly
        for(const auto& inst : vol.instances) {
            vol.bb.merge(Pointf3(inst.posA[0], inst.posA[1], inst.posA[2]));
            vol.bb.merge(Pointf3(inst.posB[0], inst.posB[1], inst.posB[2]));
        }
        
        this->volumes.push_back(vol);
    }
}


void PreviewScene3D::set_toolpaths_range(float min_z, float max_z) {
    m_max_z = max_z;
    set_z_clipping(max_z);
    Refresh(); // Trigger redraw
}

void PreviewScene3D::before_render() {
    // Shader handles clipping now
}

void PreviewScene3D::after_render() {
    // Shader handles clipping now
}



Preview3D::Preview3D(wxWindow* parent, const wxSize& size, std::shared_ptr<Slic3r::Print> _print, std::vector<PlaterObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, size, wxTAB_TRAVERSAL), print(_print), objects(_objects), model(_model), config(_config), canvas(this,size)
{
    // init GUI elements
    slider = new wxSlider(
        this, -1,
        0,                              // default
        0,                              // min
        // we set max to a bogus non-zero value because the MSW implementation of wxSlider
        // will skip drawing the slider if max <= min:
        1,                              // max
        wxDefaultPosition,
        wxDefaultSize,
        wxVERTICAL | wxSL_INVERSE
    );
    
    this->z_label = new wxStaticText(this, -1, "", wxDefaultPosition,
        wxSize(40,-1), wxALIGN_CENTRE_HORIZONTAL);
    //z_label->SetFont(Slic3r::GUI::small_font);
    
    auto* vsizer = new wxBoxSizer(wxVERTICAL);
    vsizer->Add(slider, 1, wxALL | wxEXPAND, 3);
    vsizer->Add(z_label, 0, wxALL | wxEXPAND, 3);
    
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(&canvas, 1, wxALL | wxEXPAND, 0);
    sizer->Add(vsizer, 0, wxTOP | wxBOTTOM | wxEXPAND, 5);

    this->Bind(wxEVT_SLIDER, [this](wxCommandEvent &e){ 
        if (_enabled && !layers_z.empty()) {
             int idx = slider->GetValue();
             if(idx >= 0 && idx < layers_z.size())
                 set_z(layers_z[idx]);
        }
    }); 
    this->Bind(wxEVT_CHAR, [this](wxKeyEvent &e) {
    });
    
    SetSizer(sizer);
    SetMinSize(GetSize());
    sizer->SetSizeHints(this);
    
    // init canvas
    reload_print();
    
}
void Preview3D::reload_print(){
   
    canvas.resetObjects();
    loaded = false;
    load_print();
}

void Preview3D::load_print() {
    if(loaded) {
        return;
    }
    
    // we require that there's at least one object and the posSlice step
    // is performed on all of them (this ensures that _shifted_copies was
    // populated and we know the number of layers)
    if(!print->step_done(posSlice)) {
        _enabled = false;
        slider->Hide();
        Layout();
        canvas.Refresh();  // clears canvas
        return;
    }
    
    size_t z_idx = 0;
    {
        layers_z.clear();
        // Load all objects on the plater + support material
        for(auto* object : print->objects) {
            for(auto layer : object->layers){
                layers_z.push_back(layer->print_z);
            }
            for(auto layer : object->support_layers) {
                layers_z.push_back(layer->print_z);
            }
        }
        
        _enabled = true;
        std::sort(layers_z.begin(),layers_z.end());
        slider->SetRange(0, layers_z.size()-1);
        z_idx = slider->GetValue();
        // If invalid z_idx,  move the slider to the top
        if (z_idx >= layers_z.size() || slider->GetValue() == 0) {
            slider->SetValue(layers_z.size()-1);
            z_idx = layers_z.size()-1;
        }
        slider->Show();
        Layout();
    }
    if (IsShown()) {
        canvas.load_print_toolpaths(print);
        loaded = true;
    }
    
    set_z(layers_z.at(z_idx));
}
void Preview3D::set_z(float z) {
    if(!_enabled) return;
    z_label->SetLabel(std::to_string(z));
    canvas.set_toolpaths_range(0, z);
    if(IsShown())canvas.Refresh();
}

void Preview3D::set_bed_shape(const std::vector<Point>& shape) {
    canvas.set_bed_shape(shape);
}

} } // Namespace Slic3r::GUI