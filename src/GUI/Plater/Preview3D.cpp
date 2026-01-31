#include "Preview3D.hpp"
#include <wx/event.h>
#include "libslic3r.h"
#include "ExtrusionEntity.hpp"
#include "Layer.hpp"

namespace Slic3r { namespace GUI {

#include "ExtrusionEntityCollection.hpp"

void PreviewScene3D::load_print_toolpaths(std::shared_ptr<Slic3r::Print> print) {
    this->layers.clear();
    
    // Check for empty objects
    if(print->objects.empty()) return;

    for (auto* object : print->objects) {
        // Iterate over specific copies (instances)
        for(const auto& copy : object->_shifted_copies) {
            
            for (auto* layer : object->layers) {
                LayerData layer_data;
                layer_data.z = layer->print_z;
                
                auto add_line = [&](const Point& a, const Point& b, unsigned char r, unsigned char g, unsigned char bl) {
                    Point p1 = a; p1.translate(copy);
                    Point p2 = b; p2.translate(copy);
                    
                    layer_data.verts.push_back(unscale(p1.x)); layer_data.verts.push_back(unscale(p1.y)); layer_data.verts.push_back(layer->print_z);
                    layer_data.verts.push_back(unscale(p2.x)); layer_data.verts.push_back(unscale(p2.y)); layer_data.verts.push_back(layer->print_z);
                    
                    layer_data.colors.push_back(r); layer_data.colors.push_back(g); layer_data.colors.push_back(bl); layer_data.colors.push_back(255);
                    layer_data.colors.push_back(r); layer_data.colors.push_back(g); layer_data.colors.push_back(bl); layer_data.colors.push_back(255);
                };

                // Recursive lambda need std::function or proper auto generic lambda with 'this' or decltype
                // We use a fix-point combinator or just standard recursion with a std::function wrapper
                // Simpler: define standard recursive function (not lambda) or std::function.
                std::function<void(const ExtrusionEntityCollection&, unsigned char, unsigned char, unsigned char)> process_collection;
                
                process_collection = [&](const ExtrusionEntityCollection& collection, unsigned char r, unsigned char g, unsigned char bl) {
                    for(const auto* m_ent : collection.entities) {
                         if (const auto* loop = dynamic_cast<const ExtrusionLoop*>(m_ent)) {
                             for(const auto& path : loop->paths) {
                                 for(size_t i=0; i<path.polyline.points.size()-1; ++i) 
                                     add_line(path.polyline.points[i], path.polyline.points[i+1], r,g,bl);
                                 // Close loop
                                 add_line(path.polyline.points.back(), path.polyline.points.front(), r,g,bl);
                             }
                         } else if (const auto* path = dynamic_cast<const ExtrusionPath*>(m_ent)) {
                                 for(size_t i=0; i<path->polyline.points.size()-1; ++i) 
                                     add_line(path->polyline.points[i], path->polyline.points[i+1], r,g,bl);
                         } else if (const auto* sub_collection = dynamic_cast<const ExtrusionEntityCollection*>(m_ent)) {
                                 process_collection(*sub_collection, r, g, bl);
                         }
                    }
                };

                // Loop regions
                for (auto* region : layer->regions) {
                    // Perimeters (Yellow)
                    process_collection(region->perimeters, 255, 255, 0);

                    // Infill (Red)
                    process_collection(region->fills, 255, 0, 0);
                }
                
                if(!layer_data.verts.empty())
                    this->layers.push_back(layer_data);
            }
            
            // Support Layers
            for (auto* layer : object->support_layers) {
                LayerData layer_data;
                layer_data.z = layer->print_z;
                
                 auto add_line = [&](const Point& a, const Point& b, unsigned char r, unsigned char g, unsigned char bl) {
                    Point p1 = a; p1.translate(copy);
                    Point p2 = b; p2.translate(copy);
                    
                    layer_data.verts.push_back(unscale(p1.x)); layer_data.verts.push_back(unscale(p1.y)); layer_data.verts.push_back(layer->print_z);
                    layer_data.verts.push_back(unscale(p2.x)); layer_data.verts.push_back(unscale(p2.y)); layer_data.verts.push_back(layer->print_z);
                    
                    layer_data.colors.push_back(r); layer_data.colors.push_back(g); layer_data.colors.push_back(bl); layer_data.colors.push_back(255);
                    layer_data.colors.push_back(r); layer_data.colors.push_back(g); layer_data.colors.push_back(bl); layer_data.colors.push_back(255);
                };
                
                std::function<void(const ExtrusionEntityCollection&, unsigned char, unsigned char, unsigned char)> process_collection;
                process_collection = [&](const ExtrusionEntityCollection& collection, unsigned char r, unsigned char g, unsigned char bl) {
                    for(const auto* m_ent : collection.entities) {
                         if (const auto* path = dynamic_cast<const ExtrusionPath*>(m_ent)) {
                                 for(size_t i=0; i<path->polyline.points.size()-1; ++i) 
                                     add_line(path->polyline.points[i], path->polyline.points[i+1], r,g,bl);
                         } else if (const auto* loop = dynamic_cast<const ExtrusionLoop*>(m_ent)) {
                             for(const auto& path : loop->paths) {
                                 for(size_t i=0; i<path.polyline.points.size()-1; ++i) 
                                     add_line(path.polyline.points[i], path.polyline.points[i+1], r,g,bl);
                                 add_line(path.polyline.points.back(), path.polyline.points.front(), r,g,bl);
                             }
                         } else if (const auto* sub_collection = dynamic_cast<const ExtrusionEntityCollection*>(m_ent)) {
                                 process_collection(*sub_collection, r, g, bl);
                         }
                    }
                };
                // Support fills (Green)
                process_collection(layer->support_fills, 0, 255, 0);
                process_collection(layer->support_interface_fills, 0, 255, 0);

                if(!layer_data.verts.empty())
                    this->layers.push_back(layer_data);
            }
        }
    }
}


void PreviewScene3D::set_toolpaths_range(float min_z, float max_z) {
    m_max_z = max_z;
}

void PreviewScene3D::after_render() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glLineWidth(2.0f);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    for (const auto& layer : layers) {
        if (layer.z > m_max_z) continue;

        glVertexPointer(3, GL_FLOAT, 0, layer.verts.data());
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, layer.colors.data());
        
        // Draw arrays
        // Since we pushed individual line segments for infill, using GL_LINES is safest if we organized it that way.
        // But for perimeters we pushed points. 
        // For simplicity in this first pass, let's assume GL_LINES and hope the perimeters strictly follow. 
        // Actually for perimeters I pushed all points of polyline. GL_LINE_STRIP would be better but batching is hard.
        // Let's assume GL_POINTS for valid debug? No that's ugly.
        // Let's refactor load to always push GL_LINES pairs.
        
        if (!layer.verts.empty())
            glDrawArrays(GL_LINES, 0, layer.verts.size() / 3);
    }

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_LIGHTING);
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
        //$self->set_z($self->{layers_z}[$slider->GetValue])
        //  if $self->enabled;
    }); 
    this->Bind(wxEVT_CHAR, [this](wxKeyEvent &e) {
        /*my ($s, $event) = @_;
        
        my $key = $event->GetKeyCode;
        if ($key == 85 || $key == 315) {
            $slider->SetValue($slider->GetValue + 1);
            $self->set_z($self->{layers_z}[$slider->GetValue]);
        } elsif ($key == 68 || $key == 317) {
            $slider->SetValue($slider->GetValue - 1);
            $self->set_z($self->{layers_z}[$slider->GetValue]);
        } else {
            $event->Skip;
        }*/
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
    if(loaded) return;
    
    // we require that there's at least one object and the posSlice step
    // is performed on all of them (this ensures that _shifted_copies was
    // populated and we know the number of layers)
    if(!print->step_done(posSlice)) {
        _enabled = false;
        slider->Hide();
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
            //$z_idx = @{$self->{layer_z}} ? -1 : undef;
            z_idx = slider->GetValue(); // not sure why the perl version makes z_idx invalid
        }
        slider->Show();
        Layout();
    }
    if (IsShown()) {
        // set colors
        /*canvas.color_toolpaths_by($Slic3r::GUI::Settings->{_}{color_toolpaths_by});
        if ($self->canvas->color_toolpaths_by eq 'extruder') {
            my @filament_colors = map { s/^#//; [ map $_/255, (unpack 'C*', pack 'H*', $_), 255 ] }
                @{$self->print->config->filament_colour};
            $self->canvas->colors->[$_] = $filament_colors[$_] for 0..$#filament_colors;
        } else {
            $self->canvas->colors([ $self->canvas->default_colors ]);
        }*/
        
        // load skirt and brim
        // TODO: skirt/brim
        //canvas.load_print_toolpaths(print);
        
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

/*
void set_bed_shape() {
    my ($self, $bed_shape) = @_;
    $self->canvas->set_bed_shape($bed_shape);
}
*/

} } // Namespace Slic3r::GUI

