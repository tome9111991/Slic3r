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
            m_prevent_camera_movement = true;
            moving_volume = hover_volume;
            move_start = Point(e.GetX(), e.GetY());
        }
    }
    return false;
}

bool Plate3D::mouse_up(wxMouseEvent &e){
    m_prevent_camera_movement = false;
    bool was_dragging = dragging;
    if (Scene3D::mouse_up(e)) return true;
    
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
                
                Volume vol;
                vol.color = wxColor(200,200,200);
                vol.origin = Pointf3(0,0,0);
                vol.model = std::move(model);
                vol.bb = copy.bounding_box();
                
                // OBB Support
                vol.is_obb = true;
                vol.raw_bbox = volume->mesh.bounding_box();
                
                // Convert TransformationMatrix (double) to glm::mat4 (float)
                // TransformationMatrix is row-major but GLM is column-major?
                // Wait, Slic3r::TransformationMatrix docs say:
                // Column: out' = M1 * M2 * in'
                // entries are m00, m01.. m23. 4th row is 0 0 0 1.
                // GLM expects column-major data in constructor or row-major if using make_mat4??
                // Standard glm::mat4 constructor is column-major.
                // Slic3r Matrix:
                // m00 m01 m02 m03
                // m10 m11 m12 m13
                // m20 m21 m22 m23
                // 0   0   0   1
                
                // We need to pass it to GLM as column-major (OpenGL standard)
                // Col 0: m00, m10, m20, 0
                // Col 1: m01, m11, m21, 0
                // Col 2: m02, m12, m22, 0
                // Col 3: m03, m13, m23, 1
                
                Slic3r::TransformationMatrix t = instance->get_trafo_matrix();
                
                vol.instance_transformation = glm::mat4(
                    (float)t.m00, (float)t.m10, (float)t.m20, 0.0f,
                    (float)t.m01, (float)t.m11, (float)t.m21, 0.0f,
                    (float)t.m02, (float)t.m12, (float)t.m22, 0.0f,
                    (float)t.m03, (float)t.m13, (float)t.m23, 1.0f
                );

                volumes.push_back(std::move(vol));
            }
        }
    }
    color_volumes();
    Refresh();
}

void Plate3D::color_volumes(){
    std::vector<wxColour> filament_colors;
    
    // Attempt to get filament colors from config
    if (this->config) {
        const auto& cfg = this->config->config();
        if (cfg.has("filament_colour")) {
            const auto* opt = cfg.opt<ConfigOptionStrings>("filament_colour");
            if (opt && !opt->values.empty()) {
                for (const auto& val : opt->values) {
                    filament_colors.emplace_back(val);
                }
            }
        }
    }

    if (filament_colors.empty()) {
        filament_colors.push_back(CanvasTheme::GetColors().color_parts);
    }

    unsigned int i = 0;
    for(const PlaterObject &object: objects){
        const auto &modelobj = model->objects.at(object.identifier);
        
        int obj_extruder = 0;
        if (modelobj->config.has("extruder")) {
             obj_extruder = modelobj->config.opt<ConfigOptionInt>("extruder")->value;
        }

        for(ModelInstance *instance: modelobj->instances){
            for(ModelVolume* volume: modelobj->volumes){
                auto& rendervolume = volumes.at(i);
                rendervolume.selected = object.selected;
                
                int extruder_id = obj_extruder;
                if (volume->config.has("extruder")) {
                    extruder_id = volume->config.opt<ConfigOptionInt>("extruder")->value;
                }
                
                // Map 1-based extruder ID to 0-based index
                int idx = (extruder_id > 0) ? (extruder_id - 1) : 0;
                
                // Safety check
                if (idx >= filament_colors.size()) idx = idx % filament_colors.size();
                
                rendervolume.color = filament_colors[idx];
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
