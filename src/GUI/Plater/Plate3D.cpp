#include "Plater/Plate3D.hpp"
#include "Theme/CanvasTheme.hpp"
#include "misc_ui.hpp"

namespace Slic3r { namespace GUI {

Plate3D::Plate3D(wxWindow* parent, const wxSize& size, std::vector<PlaterObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config) :
    Scene3D(parent, size), objects(_objects), model(_model), config(_config)
{ 
    // Bind the extra mouse events
    this->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) { this->mouse_down(e); });
    this->Bind(wxEVT_RIGHT_DOWN, [this](wxMouseEvent &e) { this->mouse_down(e); });
}

bool Plate3D::mouse_down(wxMouseEvent &e)
{
    if (Scene3D::mouse_down(e)) return true;
    if (hover) {
        on_select_object(hover_object);
        if (e.LeftDown()) {
            moving = true;
            moving_volume = hover_volume;
            move_start = Point(e.GetX(), e.GetY());
        }
    }
    return false;
}

bool Plate3D::mouse_up(wxMouseEvent &e){
    if (Scene3D::mouse_up(e)) return true;
    bool was_dragging = dragging;
    if(moving){
        //translate object
        moving = false;
        unsigned int i = 0;
        for(const PlaterObject &object: objects){
            const auto &modelobj = model->objects.at(object.identifier);
            for(ModelInstance *instance: modelobj->instances){
                unsigned int size = modelobj->volumes.size();
                if(i <= moving_volume && moving_volume < i+size){
                    instance->offset.translate(volumes.at(i).origin);
                    modelobj->update_bounding_box();
                    on_instances_moved();
                    Refresh();
                    return true;
                }else{
                    i+=size;
                }
            }
        }
    } else if (!was_dragging) {
        if (e.GetButton() == wxMOUSE_BTN_RIGHT) {
            if (on_right_click) {
                on_right_click(this, e.GetPosition());
            }
        } else if (e.GetButton() == wxMOUSE_BTN_LEFT) {
            if (!hover) {
                on_select_object(-1);
            }
        }
    }
    return false;
}

bool Plate3D::mouse_move(wxMouseEvent &e){
    if (Scene3D::mouse_move(e)) return true;
    if(!e.Dragging()){
        pos = Point(e.GetX(),e.GetY());
        mouse = true;
        Refresh();
    } else if(moving){
        const auto p = Point(e.GetX(),e.GetY());
        const auto current = mouse_ray(p).intersect_plane(0);
        const auto old = mouse_ray(move_start).intersect_plane(0);
        move_start = p;
        unsigned int i = 0;
        for(const PlaterObject &object: objects){
            const auto &modelobj = model->objects.at(object.identifier);
            for(ModelInstance *instance: modelobj->instances){
                unsigned int size = modelobj->volumes.size();
                if(i <= moving_volume && moving_volume < i+size){
                    for(ModelVolume* volume: modelobj->volumes){
                        volumes.at(i).origin.translate(old.vector_to(current));
                        i++;
                    }
                    Refresh();
                    return true;
                }else{
                    i+=size;
                }
            }
        }
    }
    return false;
}

void Plate3D::update(){
    volumes.clear();
    for(const PlaterObject &object: objects){
        const auto &modelobj = model->objects.at(object.identifier);
        for(ModelInstance *instance: modelobj->instances){
            for(ModelVolume* volume: modelobj->volumes){
                TriangleMesh copy = volume->mesh;
                instance->transform_mesh(&copy);
                GLVertexArray model;
                model.load_mesh(copy);
                volumes.push_back(Volume{ wxColor(200,200,200), Pointf3(0,0,0), model, copy.bounding_box()});
            }
        }
    }
    color_volumes();
    Refresh();
}

void Plate3D::color_volumes(){
    unsigned int i = 0;
    for(const PlaterObject &object: objects){
        const auto &modelobj = model->objects.at(object.identifier);

        for(ModelInstance *instance: modelobj->instances){
            for(ModelVolume* volume: modelobj->volumes){
                auto& rendervolume = volumes.at(i);
                rendervolume.selected = object.selected;
                rendervolume.color = CanvasTheme::GetColors().color_parts;
                i++;
            }
        }
    }
}

void Plate3D::before_render(){
    if (!mouse){
        color_volumes();
        return;
    }

    // Color each volume a different color, render and test which color is beneath the mouse.
    m_shader->set_uniform("u_lit", 0);
    m_shader->set_uniform("u_alpha", 1.0f);

    unsigned int i = 1;
    for(Volume &volume : volumes){
        volume.color = wxColor((i>>16)&0xFF,(i>>8)&0xFF,i&0xFF);
        i++;
    }
    draw_volumes();
    glFlush();
    glFinish();
    
    GLubyte color[4] = {0,0,0,0};
    double scale = GetContentScaleFactor();
    glReadPixels((int)(pos.x * scale), (int)((GetSize().GetHeight() - pos.y) * scale), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, color);

    // Handle the hovered volume
    unsigned int index = (color[0]<<16) + (color[1]<<8) + color[2];
    hover = false;

    if (index != 0 && index <= volumes.size()) {
        hover = true;
        hover_volume = index - 1;
        unsigned int k = 0;
        unsigned int v_idx = 0;
        for(const PlaterObject &object: objects){
            const auto &modelobj = model->objects.at(object.identifier);
            unsigned int volumes_count = modelobj->instances.size() * modelobj->volumes.size();
            if(v_idx <= hover_volume && hover_volume < v_idx + volumes_count){
                hover_object = k;
                break;
            }
            v_idx += volumes_count;
            k++;
        }
    }
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glFlush();
    glFinish();
    
    // Reset shader state for normal rendering
    m_shader->set_uniform("u_lit", 1);
    color_volumes();
    mouse = false;
}

void Plate3D::render_imgui() {
    ImGuiToolbar::draw(m_actions, (float)this->GetSize().GetWidth(), (float)this->GetSize().GetHeight());
}

} } // Namespace Slic3r::GUI
