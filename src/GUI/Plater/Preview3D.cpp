#include "Preview3D.hpp"
#include <wx/event.h>
#include "libslic3r.h"
#include "ExtrusionEntity.hpp"
#include "Layer.hpp"
#include <fstream>
#include "ExtrusionGeometry.hpp"
#include "ExtrusionEntityCollection.hpp"

namespace Slic3r { namespace GUI {

void PreviewScene3D::load_print_toolpaths(std::shared_ptr<Slic3r::Print> print) {
    this->volumes.clear();
    this->layers.clear();
    
    if(!print || print->objects.empty()) return;

    // Helper to get color for role
    auto get_color = [&](int role) -> wxColor {
        // Simple color mapping for now
        // 0: Perimeter (Yellow), 1: Infill (Red), 2: Support (Green)
        switch(role) {
            case 0: return wxColor(240, 180, 50); // Softer Gold
            case 1: return wxColor(200, 100, 70); // Terracotta (Less jarring vs Gold)
            case 2: return wxColor(100, 200, 120); // Mint Green
            default: return wxColor(150, 150, 160); // Neutral Grey
        }
    };

    // Helper to process extrusion collection recursively
    std::function<void(const ExtrusionEntityCollection&, const Point&, double, ::Slic3r::GLVertexArray&, ::Slic3r::GLVertexArray&)> process_collection;
    process_collection = [&](const ExtrusionEntityCollection& collection, const Point& copy, double top_z, ::Slic3r::GLVertexArray& qverts_out, ::Slic3r::GLVertexArray& tverts_out) {
        for(const auto* entity : collection.entities) {
            Lines lines;
            std::vector<double> widths;
            std::vector<double> heights;
            bool closed = false;

            if (const auto* path = dynamic_cast<const ExtrusionPath*>(entity)) {
                Polyline polyline = path->polyline;
                polyline.remove_duplicate_points();
                polyline.translate(copy);
                lines = polyline.lines();
                widths.assign(lines.size(), path->width);
                heights.assign(lines.size(), path->height);
                closed = false;
            } else if (const auto* loop = dynamic_cast<const ExtrusionLoop*>(entity)) {
                for(const auto& path : loop->paths) {
                    Polyline polyline = path.polyline;
                    polyline.remove_duplicate_points();
                    polyline.translate(copy);
                    Lines path_lines = polyline.lines();
                    lines.insert(lines.end(), path_lines.begin(), path_lines.end());
                    widths.insert(widths.end(), path_lines.size(), path.width);
                    heights.insert(heights.end(), path_lines.size(), path.height);
                }
                closed = true;
            } else if (const auto* sub = dynamic_cast<const ExtrusionEntityCollection*>(entity)) {
                process_collection(*sub, copy, top_z, qverts_out, tverts_out);
                continue;
            }

            if(!lines.empty()) {
                ::Slic3r::ExtrusionGeometry::_extrusionentity_to_verts_do(lines, widths, heights, closed, top_z, Point(0,0), &qverts_out, &tverts_out);
            }
        }
    };

    // Temporary storage for volume data by color
    struct VolData {
        ::Slic3r::GLVertexArray qverts;
        ::Slic3r::GLVertexArray tverts;
        std::vector<float> q_tube_x;
        std::vector<float> t_tube_x;
        BoundingBoxf3 bb;
    };
    std::map<unsigned long, VolData> vol_map; // Key: RGB value

    auto add_to_volume = [&](int role, const ExtrusionEntityCollection& collection, const Point& copy, double z) {
        wxColor c = get_color(role);
        unsigned long color_key = c.GetRGB();
        process_collection(collection, copy, z, vol_map[color_key].qverts, vol_map[color_key].tverts);
    };

    for (auto* object : print->objects) {
        for(const auto& copy : object->_shifted_copies) {
            for (auto* layer : object->layers) {
                for (auto* region : layer->regions) {
                    add_to_volume(0, region->perimeters, copy, layer->print_z);
                    add_to_volume(1, region->fills, copy, layer->print_z);
                }
            }
            for (auto* layer : object->support_layers) {
                add_to_volume(2, layer->support_fills, copy, layer->print_z);
                add_to_volume(2, layer->support_interface_fills, copy, layer->print_z);
            }
        }
    }

    // Finalize volumes
    for(auto& pair : vol_map) {
        Volume vol;
        vol.color.SetRGB(pair.first);
        vol.origin = Pointf3(0,0,0);
        
        // Convert qverts (Quads) to Triangles and merge with tverts
        ::Slic3r::GLVertexArray& q = pair.second.qverts;
        ::Slic3r::GLVertexArray& t = pair.second.tverts;
        
        vol.model.verts = t.verts;
        vol.model.norms = t.norms;
        // Init tube coords for tverts (triangles - usually caps) with 0
        vol.tube_coords.assign(t.verts.size()/3, 0.0f);
        
        // Append quads as triangles
        for(size_t i=0; i < q.verts.size(); i += 12) {
             // Quad Verts indices: i, i+1, i+2, i+3 (relative to quad start, each is 3 floats)
             
             if(i+11 >= q.verts.size()) break; // Safety check

             // Analyze Quad to determine tube coords
             float tube_x[4] = {0,0,0,0};
             float nz = q.norms[i+2]; // Normal Z of first vertex
             
             if (abs(nz) > 0.9f) {
                 // Top or Bottom face (-1, 1, 1, -1) pattern usually works for TL, TR, BR, BL winding
                 tube_x[0] = -1.0f;
                 tube_x[1] = 1.0f;
                 tube_x[2] = 1.0f;
                 tube_x[3] = -1.0f;
             } else {
                 // Side face - Use Z height to determine -1 (bottom) vs 1 (top)
                 float zs[4] = {q.verts[i+2], q.verts[i+5], q.verts[i+8], q.verts[i+11]};
                 float min_z = zs[0], max_z = zs[0];
                 for(int k=1; k<4; ++k) {
                     if(zs[k] < min_z) min_z = zs[k];
                     if(zs[k] > max_z) max_z = zs[k];
                 }
                 float range = max_z - min_z;
                 if (range < 0.001f) range = 0.001f; // Avoid divide by zero
                 
                 for(int k=0; k<4; ++k) {
                     // Normalize to 0..1 then map to -1..1
                     float h = (zs[k] - min_z) / range;
                     tube_x[k] = h * 2.0f - 1.0f;
                 }
             }

             // Triangle 1: V0, V1, V2
             for(int k=0; k<3; ++k) vol.model.verts.push_back(q.verts[i+k]);
             for(int k=0; k<3; ++k) vol.model.norms.push_back(q.norms[i+k]);
             vol.tube_coords.push_back(tube_x[0]);
             
             for(int k=0; k<3; ++k) vol.model.verts.push_back(q.verts[i+3+k]);
             for(int k=0; k<3; ++k) vol.model.norms.push_back(q.norms[i+3+k]);
             vol.tube_coords.push_back(tube_x[1]);

             for(int k=0; k<3; ++k) vol.model.verts.push_back(q.verts[i+6+k]);
             for(int k=0; k<3; ++k) vol.model.norms.push_back(q.norms[i+6+k]);
             vol.tube_coords.push_back(tube_x[2]);
             
             // Triangle 2: V0, V2, V3
             for(int k=0; k<3; ++k) vol.model.verts.push_back(q.verts[i+k]);
             for(int k=0; k<3; ++k) vol.model.norms.push_back(q.norms[i+k]);
             vol.tube_coords.push_back(tube_x[0]); // V0

             for(int k=0; k<3; ++k) vol.model.verts.push_back(q.verts[i+6+k]);
             for(int k=0; k<3; ++k) vol.model.norms.push_back(q.norms[i+6+k]);
             vol.tube_coords.push_back(tube_x[2]); // V2

             for(int k=0; k<3; ++k) vol.model.verts.push_back(q.verts[i+9+k]);
             for(int k=0; k<3; ++k) vol.model.norms.push_back(q.norms[i+9+k]);
             vol.tube_coords.push_back(tube_x[3]); // V3
        }
        
        // Compute BB
        if(!vol.model.verts.empty()) {
            for(size_t i=0; i<vol.model.verts.size(); i+=3) {
                vol.bb.merge(Pointf3(vol.model.verts[i], vol.model.verts[i+1], vol.model.verts[i+2]));
            }
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
    std::ofstream log("preview_debug.log", std::ios::app);
    log << "Preview3D::load_print called" << std::endl;
    if(loaded) {
        log << "Already loaded" << std::endl;
        return;
    }
    
    // we require that there's at least one object and the posSlice step
    // is performed on all of them (this ensures that _shifted_copies was
    // populated and we know the number of layers)
    if(!print->step_done(posSlice)) {
        log << "Slice step NOT done" << std::endl;
        _enabled = false;
        slider->Hide();
        canvas.Refresh();  // clears canvas
        return;
    }
    log << "Slice step DONE" << std::endl;
    
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
            //$z_idx = @{$self->{layer_z}} ? -1 : undef;
            z_idx = slider->GetValue(); // not sure why the perl version makes z_idx invalid
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