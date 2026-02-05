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
};

class Scene3D : public wxGLCanvas {
public:
    Scene3D(wxWindow* parent, const wxSize& size);
    ~Scene3D();
    
    void set_camera_view(Direction dir);
    void set_z_clipping(float z);

private:
    wxGLContext* m_context = nullptr;
    
    // GL State
    bool init = false;
    void init_gl();
    void init_shaders();
    
    // Camera
    GL::Camera m_camera;
    bool dragging = false;
    Point drag_start = Point(0,0);
    
    // Rendering Resources
    std::unique_ptr<GL::Shader> m_shader;
    std::unique_ptr<GL::Shader> m_shader_bg;
    
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
    Linef3 mouse_ray(Point win);
    void draw_volumes();
    void set_bed_shape(Points _bed_shape);
    
    std::vector<Volume> volumes;
    Volume load_object(ModelVolume &mv, ModelInstance &mi);
    
    // Event Handlers
    virtual void mouse_up(wxMouseEvent &e);
    virtual void mouse_move(wxMouseEvent &e);
    virtual void mouse_dclick(wxMouseEvent &e);
    virtual void mouse_wheel(wxMouseEvent &e);
    
    // Hooks
    virtual void before_render(){};
    virtual void after_render(){};
 };

} } // Namespace Slic3r::GUI
#endif