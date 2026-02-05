#include "Scene3D.hpp"
#include "Theme/CanvasTheme.hpp"
#include "Log.hpp"
#include "ExtrusionGeometry.hpp"
#include "libslic3r/ClipperUtils.hpp"

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Slic3r { namespace GUI {

static const int gl_attrs[] = {
    WX_GL_RGBA, 
    WX_GL_DOUBLEBUFFER, 
    WX_GL_DEPTH_SIZE, 24,
    0
};

Scene3D::Scene3D(wxWindow* parent, const wxSize& size) :
    wxGLCanvas(parent, wxID_ANY, gl_attrs, wxDefaultPosition, size)
{
    // Request OpenGL 4.6 Core Profile
    wxGLContextAttrs ctxAttrs;
    ctxAttrs.PlatformDefaults().OGLVersion(4, 6).CoreProfile().EndList();
    
    m_context = new wxGLContext(this, nullptr, &ctxAttrs);
    
    // Bind Events
    this->Bind(wxEVT_PAINT, [this](wxPaintEvent &e) { this->repaint(e); });
    this->Bind(wxEVT_SIZE, [this](wxSizeEvent &e ){ resize(); });
    this->Bind(wxEVT_MOTION, [this](wxMouseEvent &e) { this->mouse_move(e); });
    this->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent &e) { this->mouse_up(e); });
    this->Bind(wxEVT_RIGHT_UP, [this](wxMouseEvent &e) { this->mouse_up(e); });
    this->Bind(wxEVT_MIDDLE_DCLICK, [this](wxMouseEvent &e) { this->mouse_dclick(e); });
    this->Bind(wxEVT_MOUSEWHEEL, [this](wxMouseEvent &e) { this->mouse_wheel(e); });
    
    // Default Bed
    Points p;
    const coord_t w = scale_(200), z = 0;
    p.push_back(Point(z,z));
    p.push_back(Point(z,w));
    p.push_back(Point(w,w));
    p.push_back(Point(w,z));
    set_bed_shape(p);
}

Scene3D::~Scene3D() {
    if (m_context) {
        SetCurrent(*m_context);
        m_vbo_bed.reset();
        m_vao_bed.reset();
        m_vbo_grid.reset();
        m_vao_grid.reset();
        m_vbo_axes.reset();
        m_vao_axes.reset();
        m_vbo_bg.reset();
        m_vao_bg.reset();
        m_shader.reset();
        m_shader_bg.reset();
        volumes.clear();
    }
    delete m_context;
}

void Scene3D::init_gl(){
    if(this->init) return;
    
    // Initialize GLAD
    if(!GL::GLManager::init()) {
        Slic3r::Log::error("Scene3D", "Failed to init GLAD");
        return;
    }
    
    this->init = true;
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    
    if (!m_vbo_bed) m_vbo_bed = std::make_unique<GL::VertexBuffer>();
    if (!m_vao_bed) m_vao_bed = std::make_unique<GL::VertexArray>();
    if (!m_vbo_grid) m_vbo_grid = std::make_unique<GL::VertexBuffer>();
    if (!m_vao_grid) m_vao_grid = std::make_unique<GL::VertexArray>();
    if (!m_vbo_axes) m_vbo_axes = std::make_unique<GL::VertexBuffer>();
    if (!m_vao_axes) m_vao_axes = std::make_unique<GL::VertexArray>();
    if (!m_vbo_bg) m_vbo_bg = std::make_unique<GL::VertexBuffer>();
    if (!m_vao_bg) m_vao_bg = std::make_unique<GL::VertexArray>();

    init_shaders();
}

void Scene3D::init_shaders() {
    m_shader = std::make_unique<GL::Shader>("MainShader");
    
    std::string vs = R"(
        #version 460 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in float aTube;
        
        uniform mat4 view;
        uniform mat4 projection;
        uniform mat4 model;
        
        out vec3 v_frag_pos;
        out vec3 v_normal;
        out float v_z_height;
        out float v_tube;
        
        void main() {
            vec4 worldPos = model * vec4(aPos, 1.0);
            v_frag_pos = vec3(worldPos);
            v_normal = mat3(transpose(inverse(model))) * aNormal;
            v_z_height = aPos.z;
            v_tube = aTube;
            gl_Position = projection * view * worldPos;
        }
    )";
    
    std::string fs = R"(
        #version 460 core
        out vec4 FragColor;
        
        in vec3 v_frag_pos;
        in vec3 v_normal;
        in float v_z_height;
        in float v_tube;
        
        uniform vec3 objectColor;
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform float u_clipping_z;
        uniform int u_lit; // 1 = lit, 0 = flat
        
        void main() {
            if (v_z_height > u_clipping_z) discard;
            
            if (u_lit == 0) {
                FragColor = vec4(objectColor, 1.0);
                return;
            }
        
            vec3 N = normalize(v_normal);
            // ... (rest same, omitting for brevity in actual tool call but I should include enough)
            if (abs(v_tube) > 0.0) {
                float flat_region = 0.6;
                float side_coord = (abs(v_tube) - flat_region) / (1.0 - flat_region);
                if (side_coord > 0.0) {
                    float angle = side_coord * 1.5708;
                    float tilt_z = sin(angle) * sign(v_tube);
                    N = normalize(vec3(tilt_z, 0.0, 1.0 - abs(tilt_z)*0.5));
                }
            }

            vec3 lightDir = normalize(vec3(0.2, 0.2, 1.0)); 
            vec3 viewDir = normalize(viewPos - v_frag_pos);
            float diff = max(dot(N, lightDir), 0.0);
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(N, halfwayDir), 0.0), 32.0);
            float ao = 1.0 - smoothstep(0.7, 1.0, abs(v_tube)) * 0.4;
            
            vec3 ambient = 0.3 * objectColor; // Lowered ambient
            vec3 diffuse = diff * objectColor;
            vec3 specular = vec3(0.2) * spec; // Lowered specular for less "washed out" look
            
            vec3 result = (ambient + diffuse + specular) * ao;
            result *= (0.85 + 0.15 * smoothstep(-0.1, 0.1, v_z_height));
            result = pow(result, vec3(1.0/2.2));
            FragColor = vec4(result, 1.0);
        }
    )";
    
    if(!m_shader->init(vs, fs)) {
        Slic3r::Log::error("Scene3D", "Shader init failed");
    }

    // BG Shader
    m_shader_bg = std::make_unique<GL::Shader>("BGShader");
    std::string bg_vs = R"(
        #version 460 core
        layout (location = 0) in vec2 aPos;
        out vec2 v_pos;
        void main() {
            v_pos = aPos;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";
    std::string bg_fs = R"(
        #version 460 core
        out vec4 FragColor;
        in vec2 v_pos;
        uniform vec3 colorTop;
        uniform vec3 colorBottom;
        void main() {
            float t = v_pos.y * 0.5 + 0.5;
            FragColor = vec4(mix(colorBottom, colorTop, t), 1.0);
        }
    )";
    m_shader_bg->init(bg_vs, bg_fs);

    // Init BG Quad
    float bg_quad[] = { -1,-1, 1,-1, 1,1, -1,1 };
    m_vao_bg->bind();
    m_vbo_bg->upload_data(bg_quad, sizeof(bg_quad));
    m_vao_bg->add_attribute(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), 0);
}

void Scene3D::resize(){
    const auto s = GetSize();
    if (s.GetWidth() <= 0 || s.GetHeight() <= 0) return;
    m_camera.set_viewport(0, 0, s.GetWidth(), s.GetHeight());
    if (IsShownOnScreen() && m_context) {
        SetCurrent(*m_context);
        init_gl();
        if (GL::GLManager::is_initialized()) {
            glViewport(0, 0, s.GetWidth(), s.GetHeight());
        }
    }

   
    Refresh();
}

void Scene3D::repaint(wxPaintEvent& e) {
    if(!IsShownOnScreen()) return;
    SetCurrent(*m_context);
    
    init_gl();
    
    // Update Bed VBOs if needed
    if(m_bed_dirty) {
        // Bed (Triangles)
        std::vector<float> bed_data;
        for(size_t i=0; i<bed_verts.size(); i+=3) {
            bed_data.push_back(bed_verts[i]);
            bed_data.push_back(bed_verts[i+1]);
            bed_data.push_back(bed_verts[i+2]);
            // Normal (0,0,1)
            bed_data.push_back(0.0f);
            bed_data.push_back(0.0f);
            bed_data.push_back(1.0f);
            // Tube (0)
            bed_data.push_back(0.0f);
        }
        m_vbo_bed->upload_data(bed_data.data(), bed_data.size() * sizeof(float));
        m_vbo_bed->set_count(bed_verts.size() / 3);
        
        // Grid (Lines)
        std::vector<float> grid_data;
        for(size_t i=0; i<grid_verts.size(); i+=3) {
            grid_data.push_back(grid_verts[i]);
            grid_data.push_back(grid_verts[i+1]);
            grid_data.push_back(grid_verts[i+2]);
            // Normal (dummy)
            grid_data.push_back(0.0f);
            grid_data.push_back(0.0f);
            grid_data.push_back(1.0f);
            // Tube (0)
            grid_data.push_back(0.0f);
        }
        m_vbo_grid->upload_data(grid_data.data(), grid_data.size() * sizeof(float));
        m_vbo_grid->set_count(grid_verts.size() / 3);
        
        // Setup VAOs
        m_vao_bed->bind();
        m_vbo_bed->bind();
        m_vao_bed->add_attribute(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        m_vao_bed->add_attribute(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        m_vao_bed->add_attribute(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
        
        m_vao_grid->add_attribute(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        m_vao_grid->add_attribute(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
        
        // Axes (Lines)
        std::vector<float> axes_data;
        float ax_l = 50.0f; // Longer axes
        // X (Red)
        axes_data.insert(axes_data.end(), { 0,0,0, 1,0,0, 0,  ax_l,0,0, 1,0,0, 0 });
        // Y (Green)
        axes_data.insert(axes_data.end(), { 0,0,0, 0,1,0, 0,  0,ax_l,0, 0,1,0, 0 });
        // Z (Blue)
        axes_data.insert(axes_data.end(), { 0,0,0, 0,0,1, 0,  0,0,ax_l, 0,0,1, 0 });
        
        m_vbo_axes->upload_data(axes_data.data(), axes_data.size()*sizeof(float));
        m_vbo_axes->set_count(6);
        m_vao_axes->bind();
        m_vbo_axes->bind();
        m_vao_axes->add_attribute(0, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), 0);
        m_vao_axes->add_attribute(1, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(3*sizeof(float)));
        
        m_bed_dirty = false;
    }
    
    // Clear
    auto colors = CanvasTheme::GetColors();
    wxColour bg = colors.canvas_bg_top;
    glClearColor(bg.Red()/255.0f, bg.Green()/255.0f, bg.Blue()/255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if(!m_shader) return;
    
    m_shader->bind();
    
    // Uniforms
    glm::mat4 view = m_camera.get_view_matrix();
    glm::mat4 projection = m_camera.get_projection_matrix();
    glm::vec3 camPos = m_camera.get_position();
    
    m_shader->set_uniform("view", view);
    m_shader->set_uniform("projection", projection);
    m_shader->set_uniform("viewPos", camPos);
    m_shader->set_uniform("lightPos", glm::vec3(0.0f, 0.0f, 100.0f)); 
    m_shader->set_uniform("u_clipping_z", m_clipping_z);
    m_shader->set_uniform("u_lit", 1);
    
    draw_background();
    this->before_render();

    draw_ground();
    draw_volumes();
    
    // Draw Axes (Unlit)
    m_shader->set_uniform("u_lit", 0);
    draw_axes(Pointf3(0,0,0), 20.0f, 2, true);

    this->after_render();

    glFlush();
    SwapBuffers();
}

void Scene3D::draw_ground() {
    auto colors = CanvasTheme::GetColors();
    auto ground = colors.ground_color;
    auto grid = colors.grid_color;
    
    glm::mat4 model = glm::mat4(1.0f);
    m_shader->set_uniform("model", model);
    
    // Bed
    m_shader->set_uniform("objectColor", glm::vec3(ground.Red()/255.0f, ground.Green()/255.0f, ground.Blue()/255.0f));
    m_vao_bed->bind();
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vbo_bed->get_count());
    
    // Grid
    m_shader->set_uniform("u_lit", 0); // Grid should be flat
    m_shader->set_uniform("objectColor", glm::vec3(grid.Red()/255.0f, grid.Green()/255.0f, grid.Blue()/255.0f));
    m_vao_grid->bind();
    glDrawArrays(GL_LINES, 0, (GLsizei)m_vbo_grid->get_count());
    m_vao_grid->unbind();
    m_shader->set_uniform("u_lit", 1);
}

void Scene3D::draw_volumes() {
    for(auto &volume : volumes) {
        if(volume.gpu_dirty) {
            // Create VBO/VAO
            volume.vbo = std::make_shared<GL::VertexBuffer>();
            volume.vao = std::make_shared<GL::VertexArray>();
            
            // Interleave: Pos(3), Normal(3), Tube(1) = 7 floats per vertex
            std::vector<float> data;
            size_t count = volume.model.verts.size() / 3; 
            for(size_t i=0; i<count; ++i) {
                // Pos
                data.push_back(volume.model.verts[i*3]);
                data.push_back(volume.model.verts[i*3+1]);
                data.push_back(volume.model.verts[i*3+2]);
                // Norm
                if(volume.model.norms.size() > i*3+2) {
                    data.push_back(volume.model.norms[i*3]);
                    data.push_back(volume.model.norms[i*3+1]);
                    data.push_back(volume.model.norms[i*3+2]);
                } else {
                    data.push_back(0); data.push_back(0); data.push_back(1);
                }
                // Tube Coords
                if (volume.tube_coords.size() > i) {
                    data.push_back(volume.tube_coords[i]);
                } else {
                    data.push_back(0.0f);
                }
            }
            
            volume.vbo->upload_data(data.data(), data.size() * sizeof(float));
            volume.vbo->set_count(count);
            
            volume.vao->bind();
            volume.vbo->bind();
            volume.vao->add_attribute(0, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)0);
            volume.vao->add_attribute(1, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(3*sizeof(float)));
            volume.vao->add_attribute(2, 1, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(6*sizeof(float)));
            
            volume.gpu_dirty = false;
        }
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(volume.origin.x, volume.origin.y, volume.origin.z));
        
        m_shader->set_uniform("model", model);
        m_shader->set_uniform("objectColor", glm::vec3(volume.color.Red()/255.0f, volume.color.Green()/255.0f, volume.color.Blue()/255.0f));
        
        volume.vao->bind();
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)volume.vbo->get_count());
    }
}

void Scene3D::set_bed_shape(Points _bed_shape){
    bed_shape = _bed_shape;
    const float GROUND_Z = -0.02f;
    
    const auto expoly = ExPolygon(Polygon(bed_shape));
    const auto box = expoly.bounding_box();
    bed_bound = box;
    
    // Update camera target
    Point center = bed_bound.center();
    m_camera.set_target(glm::vec3(unscale(center.x), unscale(center.y), 0.0f));

    // Triangulate
    {
        std::vector<Polygon> triangles;
        expoly.triangulate(&triangles);
        bed_verts.clear();
        for(const auto &triangle : triangles){
            for(const auto &point : triangle.points){
                bed_verts.push_back(unscale(point.x));
                bed_verts.push_back(unscale(point.y));
                bed_verts.push_back(GROUND_Z);
            }
        }
    }
    // Grid
    {
        std::vector<Polyline> lines;
        for (coord_t x = box.min.x; x <= box.max.x; x += scale_(10)) {
            lines.push_back(Polyline());
            lines.back().append(Point(x,box.min.y));
            lines.back().append(Point(x,box.max.y));
        }
        for (coord_t y = box.min.y; y <= box.max.y; y += scale_(10)) {
            lines.push_back(Polyline());
            lines.back().append(Point(box.min.x,y));
            lines.back().append(Point(box.max.x,y));
        }
        
        grid_verts.clear();
        const Polylines clipped = intersection_pl(lines, to_polygons(offset_ex(expoly,SCALED_EPSILON).at(0)));
        for(const Polyline &line : clipped){
            for(const Point &point : line.points){
                grid_verts.push_back(unscale(point.x));
                grid_verts.push_back(unscale(point.y));
                grid_verts.push_back(GROUND_Z);
            }
        }
        // Bed contours
        for(const Line &line : expoly.lines()){
            grid_verts.push_back(unscale(line.a.x));
            grid_verts.push_back(unscale(line.a.y));
            grid_verts.push_back(GROUND_Z);
            grid_verts.push_back(unscale(line.b.x));
            grid_verts.push_back(unscale(line.b.y));
            grid_verts.push_back(GROUND_Z);
        }
    }
    m_bed_dirty = true;
    Refresh();
}

void Scene3D::mouse_move(wxMouseEvent &e){
    if(e.Dragging()){
        const auto pos = Point(e.GetX(),e.GetY());
        if(dragging){
             if (e.LeftIsDown()) {
                // Rotate
                float dx = (pos.x - drag_start.x);
                float dy = (pos.y - drag_start.y);
                m_camera.rotate(dy * 0.25f, dx * 0.25f);
            } else if (e.MiddleIsDown() || e.RightIsDown()) {
                // Pan
                float dx = (pos.x - drag_start.x);
                float dy = (pos.y - drag_start.y);
                m_camera.pan(dx * 0.5f, dy * 0.5f);
            }
            Refresh();
        }
        dragging = true;
        drag_start = pos; 
    }else{
        e.Skip();
    }
}

void Scene3D::mouse_up(wxMouseEvent &e){
    dragging = false;
    Refresh();
}

void Scene3D::mouse_wheel(wxMouseEvent &e){
    float delta = ((float)e.GetWheelRotation()) / e.GetWheelDelta();
    // delta is typically 1 or -1
    // We want a multiplier - reduced speed (5% instead of 10%)
    float zoom_factor = (delta > 0) ? 1.05f : 0.95f;
    m_camera.zoom(zoom_factor);
    Refresh();
}

void Scene3D::mouse_dclick(wxMouseEvent &e){
    // Reset view?
    Refresh();
}

Linef3 Scene3D::mouse_ray(Point win){
    glm::vec3 orig, dir;
    m_camera.screen_to_world(win.x, win.y, orig, dir);
    // Return a Linef3 (segment) that is long enough
    return Linef3(Pointf3(orig.x, orig.y, orig.z), Pointf3(orig.x + dir.x*1000.0f, orig.y + dir.y*1000.0f, orig.z + dir.z*1000.0f));
}

void Scene3D::draw_axes(Pointf3 center, float length, int width, bool always_visible) {
    if (always_visible) glDisable(GL_DEPTH_TEST);
    
    glLineWidth(width);
    m_vao_axes->bind();
    
    // We use vertex attribute 1 (normal slot) as Color in unlit mode
    // X Axis
    m_shader->set_uniform("objectColor", glm::vec3(1,0,0));
    glDrawArrays(GL_LINES, 0, 2);
    // Y Axis
    m_shader->set_uniform("objectColor", glm::vec3(0,1,0));
    glDrawArrays(GL_LINES, 2, 2);
    // Z Axis
    m_shader->set_uniform("objectColor", glm::vec3(0,0,1));
    glDrawArrays(GL_LINES, 4, 2);
    
    if (always_visible) glEnable(GL_DEPTH_TEST);
}

void Scene3D::draw_background() {
    auto colors = CanvasTheme::GetColors();
    glDisable(GL_DEPTH_TEST);
    m_shader_bg->bind();
    m_shader_bg->set_uniform("colorTop", glm::vec3(colors.canvas_bg_top.Red()/255.0f, colors.canvas_bg_top.Green()/255.0f, colors.canvas_bg_top.Blue()/255.0f));
    m_shader_bg->set_uniform("colorBottom", glm::vec3(colors.canvas_bg_bottom.Red()/255.0f, colors.canvas_bg_bottom.Green()/255.0f, colors.canvas_bg_bottom.Blue()/255.0f));
    
    m_vao_bg->bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    m_shader->bind(); // back to main
    glEnable(GL_DEPTH_TEST);
}

// Stub for load_object
Volume Scene3D::load_object(ModelVolume &mv, ModelInstance &mi) {
    Volume vol;
    vol.origin = Pointf3(mi.offset.x, mi.offset.y, 0.0f);
    // Load mesh logic would go here
    return vol;
}

void Scene3D::set_camera_view(Direction dir) {
    // TODO: Map direction to camera angles
    Refresh();
}

void Scene3D::set_z_clipping(float z) {
    m_clipping_z = z;
    Refresh();
}


} } // Namespace Slic3r::GUI