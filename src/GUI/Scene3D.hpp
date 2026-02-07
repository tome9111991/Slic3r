#ifndef SCENE3D_HPP
#define SCENE3D_HPP

#include "GL/GLManager.hpp"
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
    
#include <wx/glcanvas.h>
#include "Settings.hpp"
#include "Point.hpp"
#include "ExtrusionGeometry.hpp"
#include "BoundingBox.hpp"
#include "Model.hpp"

// Modern GL
#include "GL/Shader.hpp"
#include "GL/Camera.hpp"
#include "GL/Buffer.hpp"
#include <memory>
#include "ImGuiWrapper.hpp"

namespace Slic3r { namespace GUI {

enum class Direction { Top, Bottom, Left, Right, Front, Back, Diagonal };

struct Volume {
    wxColor color;
    Pointf3 origin;
    GLVertexArray model; // CPU Data
    BoundingBoxf3 bb;
    std::vector<float> tube_coords;
    bool selected = false;

    // GPU Data
    std::shared_ptr<GL::VertexBuffer> vbo;
    std::shared_ptr<GL::VertexArray> vao;
    bool gpu_dirty = true;
    
    // Instancing
    bool is_instanced = false;
    std::vector<ExtrusionGeometry::InstanceData> instances;
    std::shared_ptr<GL::VertexBuffer> instance_buffer;
    size_t instance_count = 0;
};

class Scene3D : public wxGLCanvas {
public:
    Scene3D(wxWindow* parent, const wxSize& size);
    ~Scene3D();
    
    void set_camera_view(Direction dir);
    void set_z_clipping(float z);
    
    void init_gl();
    wxGLContext* GetContext() { return m_context; }

private:
    wxGLContext* m_context = nullptr;
    std::unique_ptr<ImGuiWrapper> m_imgui;
    
    // GL State
    bool init = false;
    void init_shaders();
    
protected:
    GL::Camera m_camera;
    bool dragging = false;
    Point drag_start = Point(0,0);

    
    // Bed Geometry
    Points bed_shape;
    BoundingBox bed_bound;
    std::unique_ptr<GL::VertexBuffer> m_vbo_bed;
    std::unique_ptr<GL::VertexArray> m_vao_bed;
    std::unique_ptr<GL::VertexBuffer> m_vbo_grid;
    std::unique_ptr<GL::VertexArray> m_vao_grid;
    std::unique_ptr<GL::VertexBuffer> m_vbo_axes;
    std::unique_ptr<GL::VertexArray> m_vao_axes;
    std::unique_ptr<GL::VertexBuffer> m_vbo_bg;
    std::unique_ptr<GL::VertexArray> m_vao_bg;
    std::unique_ptr<GL::VertexBuffer> m_vbo_selection;
    std::unique_ptr<GL::VertexArray> m_vao_selection;

    bool m_bed_dirty = true;
    bool m_axes_dirty = true;
    std::vector<float> bed_verts;
    std::vector<float> grid_verts;
    
    float m_clipping_z = 10000.0f;
    
    // Internal Rendering Methods
    void repaint(wxPaintEvent &e); 
    void resize();                 
    void draw_background();
    void draw_ground();
    void draw_axes(Pointf3 center, float length, int width, bool alwaysvisible);
    
protected:
    // Shaders
    std::unique_ptr<GL::Shader> m_shader;
    std::unique_ptr<GL::Shader> m_shader_bg;
    std::unique_ptr<GL::Shader> m_shader_instanced;
    
    // Instancing Template
    std::unique_ptr<GL::VertexBuffer> m_vbo_instanced_template;
    std::unique_ptr<GL::VertexArray> m_vao_instanced_template;

    void draw_selection_box(const BoundingBoxf3& bb);

    Linef3 mouse_ray(Point win);
    void draw_volumes();
    void set_bed_shape(Points _bed_shape);
    
    std::vector<Volume> volumes;
    Volume load_object(ModelVolume &mv, ModelInstance &mi);
    
    // Event Handlers
    virtual bool mouse_up(wxMouseEvent& e);
    virtual bool mouse_move(wxMouseEvent& e);
    virtual bool mouse_down(wxMouseEvent& e);
    virtual bool mouse_dclick(wxMouseEvent& e);
    virtual bool mouse_wheel(wxMouseEvent& e);
    
    // Hooks
    virtual void before_render(){};
    virtual void after_render(){};
    virtual void render_imgui(){};
 };

} } // Namespace Slic3r::GUI
#endif