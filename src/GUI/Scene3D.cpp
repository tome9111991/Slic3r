#include "Scene3D.hpp"
#include <cmath>
#include "Theme/CanvasTheme.hpp"
#include "Log.hpp"
#include "ExtrusionGeometry.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/Utils.hpp"
#include <fstream>
#include <sstream>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Slic3r { namespace GUI {

static const int gl_attrs[] = {
    WX_GL_RGBA, 
    WX_GL_DOUBLEBUFFER, 
    WX_GL_DEPTH_SIZE, 24,
    WX_GL_SAMPLE_BUFFERS, 1,
    WX_GL_SAMPLES, 4,
    0
};

Scene3D::Scene3D(wxWindow* parent, const wxSize& size) :
    wxGLCanvas(parent, wxID_ANY, gl_attrs, wxDefaultPosition, size),
    m_imgui(nullptr)
{ 
    // Request OpenGL 4.6 Core Profile
    wxGLContextAttrs ctxAttrs;
    ctxAttrs.PlatformDefaults().OGLVersion(4, 6).CoreProfile().EndList();
    
    m_context = new wxGLContext(this, nullptr, &ctxAttrs);
    
    // Bind Events
    this->Bind(wxEVT_PAINT, [this](wxPaintEvent &e) { this->repaint(e); });
    this->Bind(wxEVT_SIZE, [this](wxSizeEvent &e ){ resize(); });
    this->Bind(wxEVT_MOTION, [this](wxMouseEvent &e) { this->mouse_move(e); });
    this->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) { this->mouse_down(e); });
    this->Bind(wxEVT_RIGHT_DOWN, [this](wxMouseEvent &e) { this->mouse_down(e); });
    this->Bind(wxEVT_MIDDLE_DOWN, [this](wxMouseEvent &e) { this->mouse_down(e); });
    this->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent &e) { this->mouse_up(e); });
    this->Bind(wxEVT_RIGHT_UP, [this](wxMouseEvent &e) { this->mouse_up(e); });
    this->Bind(wxEVT_MIDDLE_UP, [this](wxMouseEvent &e) { this->mouse_up(e); });
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
        m_context->SetCurrent(*this);
        m_vbo_bed.reset();
        m_vao_bed.reset();
        m_vbo_grid.reset();
        m_vao_grid.reset();
        m_vbo_axes.reset();
        m_vao_axes.reset();
        m_vbo_bg.reset();
        m_vao_bg.reset();
        m_vbo_selection.reset();
        m_vao_selection.reset();
        m_shader.reset();
        m_shader_bg.reset();
        m_shader_instanced.reset();
        m_vbo_instanced_template.reset();
        m_vao_instanced_template.reset();
        volumes.clear();
    }
    delete m_context;
}

void Scene3D::init_gl(){
    if(this->init) return;

    // Ensure we have a context and it's current
    if (!m_context) return;
    
    if (!m_imgui) m_imgui = std::make_unique<ImGuiWrapper>();
    
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
    glEnable(GL_MULTISAMPLE);
    
    
    if (!m_vbo_bed) m_vbo_bed = std::make_unique<GL::VertexBuffer>();
    if (!m_vao_bed) m_vao_bed = std::make_unique<GL::VertexArray>();
    if (!m_vbo_grid) m_vbo_grid = std::make_unique<GL::VertexBuffer>();
    if (!m_vao_grid) m_vao_grid = std::make_unique<GL::VertexArray>();
    if (!m_vbo_axes) m_vbo_axes = std::make_unique<GL::VertexBuffer>();
    if (!m_vao_axes) m_vao_axes = std::make_unique<GL::VertexArray>();
    if (!m_vbo_bg) m_vbo_bg = std::make_unique<GL::VertexBuffer>();
    if (!m_vao_bg) m_vao_bg = std::make_unique<GL::VertexArray>();
    if (!m_vbo_selection) m_vbo_selection = std::make_unique<GL::VertexBuffer>();
    if (!m_vao_selection) m_vao_selection = std::make_unique<GL::VertexArray>();

    if (!m_vbo_selection) m_vbo_selection = std::make_unique<GL::VertexBuffer>();
    if (!m_vao_selection) m_vao_selection = std::make_unique<GL::VertexArray>();
    if (!m_vbo_instanced_template) m_vbo_instanced_template = std::make_unique<GL::VertexBuffer>();
    if (!m_vao_instanced_template) m_vao_instanced_template = std::make_unique<GL::VertexArray>();

    init_shaders();

    // Initial viewport setup if possible (useful during splash screen init)
    const auto s = GetSize();
    if (s.GetWidth() > 0 && s.GetHeight() > 0) {
        m_camera.set_viewport(0, 0, s.GetWidth(), s.GetHeight());
        double scale = GetContentScaleFactor();
        glViewport(0, 0, (int)(s.GetWidth() * scale), (int)(s.GetHeight() * scale));
    }
}

void Scene3D::init_shaders() {
    m_shader = std::make_unique<GL::Shader>("MainShader");
    
    auto read_file = [](const std::string& path) -> std::string {
        std::ifstream in(path);
        if(!in.is_open()) return "";
        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    };
    
    std::string shaders_dir = Slic3r::resources_dir() + "/shaders/";
    std::string vs = read_file(shaders_dir + "gouraud_light.vs");
    std::string fs = read_file(shaders_dir + "gouraud_light.fs");
    
    if (vs.empty() || fs.empty()) {
        Slic3r::Log::error("Scene3D", "Failed to load shaders from " + shaders_dir);
    }
    
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
    m_vao_bg->add_attribute(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), 0);
    
    // Instanced Shader
    m_shader_instanced = std::make_unique<GL::Shader>("InstancedShader");
    std::string ivs = read_file(shaders_dir + "instanced.vs");
    std::string ifs = read_file(shaders_dir + "instanced.fs");
    if(!m_shader_instanced->init(ivs, ifs)) {
        Slic3r::Log::error("Scene3D", "Instanced Shader init failed");
    }

    // Init Instanced Template (Unit Octagon Tube along Z, 0 to 1)
    std::vector<float> t_data;
    const int N = 8;
    struct Vec2 { float x, y; };
    std::vector<Vec2> ps(N);
    std::vector<Vec2> ns(N);

    for(int i=0; i<N; ++i) {
        float angle = (float)i * 2.0f * 3.14159265f / (float)N;
        ps[i].x = cos(angle);
        ps[i].y = sin(angle);
        // Smooth normals (same as position direction)
        ns[i] = ps[i];
    }
    
    for(int i=0; i<N; ++i) {
        int next = (i+1)%N;
        
        auto push = [&](const Vec2& p, float z, const Vec2& n) {
            t_data.push_back(p.x); t_data.push_back(p.y); t_data.push_back(z);
            t_data.push_back(n.x); t_data.push_back(n.y); t_data.push_back(0); // Normal z=0 (approx)
        };
        
        // 2 Triangles per face
        // Bottom-Left, Bottom-Right, Top-Left
        push(ps[i], 0, ns[i]);    push(ps[next], 0, ns[next]); push(ps[i], 1, ns[i]);
        // Top-Left, Bottom-Right, Top-Right
        push(ps[i], 1, ns[i]);    push(ps[next], 0, ns[next]); push(ps[next], 1, ns[next]);
    }
    
    m_vbo_instanced_template->upload_data(t_data.data(), t_data.size() * sizeof(float));
    m_vbo_instanced_template->set_count(t_data.size() / 6);
    
    m_vao_instanced_template->bind();
    m_vbo_instanced_template->bind();
    m_vao_instanced_template->add_attribute(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), 0);
    m_vao_instanced_template->add_attribute(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
}

void Scene3D::resize(){
    const auto s = GetSize();
    if (s.GetWidth() <= 0 || s.GetHeight() <= 0) return;
    m_camera.set_viewport(0, 0, s.GetWidth(), s.GetHeight());
    if (IsShownOnScreen() && m_context) {
        SetCurrent(*m_context);
        init_gl();
        if (GL::GLManager::is_initialized()) {
            double scale = GetContentScaleFactor();
            glViewport(0, 0, (int)(s.GetWidth() * scale), (int)(s.GetHeight() * scale));
        }
    }

   
    Refresh();
}

void Scene3D::repaint(wxPaintEvent& e) {
    if(!IsShownOnScreen()) return;
    SetCurrent(*m_context);
    
    init_gl();
    if (!this->init) return;

    // Ensure viewport is correct before drawing
    const auto s = GetSize();
    double scale = GetContentScaleFactor();
    glViewport(0, 0, (int)(s.GetWidth() * scale), (int)(s.GetHeight() * scale));
    m_camera.set_viewport(0, 0, s.GetWidth(), s.GetHeight());

    if (m_imgui) m_imgui->new_frame(this->GetSize().GetWidth(), this->GetSize().GetHeight());
    
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
        
        m_vao_grid->bind();
        m_vbo_grid->bind();
        m_vao_grid->add_attribute(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        m_vao_grid->add_attribute(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        m_vao_grid->add_attribute(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
        
        // Axes (Lines)
        std::vector<float> axes_data;
        float ax_l = 20.0f; 
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
    // m_shader->set_uniform("viewPos", camPos); // Unused in new shader
    // m_shader->set_uniform("lightPos", glm::vec3(0.0f, 0.0f, 100.0f)); // Unused in new shader
    m_shader->set_uniform("u_clipping_z", m_clipping_z);
    m_shader->set_uniform("u_alpha", 1.0f);
    m_shader->set_uniform("u_lit", 1);
    
    draw_background();
    this->before_render();

    draw_ground();

    draw_volumes();

    // Selection boxes pass: Occluded by parts
    // Lift the selection box slightly (+0.05f) to avoid Z-fighting with the grid at Z=0
    for(auto &volume : volumes) {
        if (volume.selected) {
            if (volume.is_obb) {
                glm::mat4 m = volume.instance_transformation;
                m[3][2] += 0.05f; // Add small Z bias
                m_shader->set_uniform("model", m);
                draw_selection_box(volume.raw_bbox);
            } else {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(volume.origin.x, volume.origin.y, volume.origin.z + 0.05f));
                m_shader->set_uniform("model", model);
                draw_selection_box(volume.bb);
            }
        }
    }
    
    // Draw Axes (Unlit)
    m_shader->set_uniform("u_lit", 0);
    draw_axes(Pointf3(0,0,0), 20.0f, 2, true);

    this->after_render();

    // Draw ImGui
    if (m_imgui) {
         this->render_imgui();
         m_imgui->render();
    }

    glFlush();
    SwapBuffers();
}

void Scene3D::draw_ground() {
    auto colors = CanvasTheme::GetColors();
    auto ground = colors.ground_color;
    auto grid = colors.grid_color;
    
    glm::mat4 model = glm::mat4(1.0f);
    m_shader->set_uniform("model", model);
    
    // Bed (Transparent Triangles)
    m_shader->set_uniform("u_lit", 0); 
    m_shader->set_uniform("objectColor", glm::vec3(ground.Red()/255.0f, ground.Green()/255.0f, ground.Blue()/255.0f));
    m_shader->set_uniform("u_alpha", ground.Alpha()/255.0f);
    
    m_vao_bed->bind();
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vbo_bed->get_count());
    
    // Grid (Flat Lines)
    glLineWidth(2.0f);
    m_shader->set_uniform("objectColor", glm::vec3(grid.Red()/255.0f, grid.Green()/255.0f, grid.Blue()/255.0f));
    m_shader->set_uniform("u_alpha", grid.Alpha()/255.0f);
    
    m_vao_grid->bind();
    glDrawArrays(GL_LINES, 0, (GLsizei)m_vbo_grid->get_count());
    m_vao_grid->unbind();
    
    // Reset uniforms
    m_shader->set_uniform("u_lit", 1);
    m_shader->set_uniform("u_alpha", 1.0f);
}

void Scene3D::draw_volumes() {
    for(auto &volume : volumes) {
        if (volume.is_instanced) {
             // --- Instanced Render Path ---
             m_shader_instanced->bind();
             
             // Set Uniforms (since we switched programs)
             glm::mat4 view = m_camera.get_view_matrix();
             glm::mat4 projection = m_camera.get_projection_matrix();
             m_shader_instanced->set_uniform("view", view);
             m_shader_instanced->set_uniform("projection", projection);
             m_shader_instanced->set_uniform("u_lit", 1);
             // m_shader_instanced->set_uniform("u_clipping_z", m_clipping_z); // Not used anymore (CPU masking)
             
             glm::mat4 model = glm::mat4(1.0f);
             model = glm::translate(model, glm::vec3(volume.origin.x, volume.origin.y, volume.origin.z));
             m_shader_instanced->set_uniform("model", model);

             if(volume.gpu_dirty) {
                 if(!volume.instance_buffer) volume.instance_buffer = std::make_shared<GL::VertexBuffer>();
                 // Upload SSBO
                 // Note: InstanceData size is 48 bytes
                 volume.instance_buffer->upload_data(volume.instances.data(), volume.instances.size() * sizeof(ExtrusionGeometry::InstanceData), GL_DYNAMIC_DRAW);
                 volume.instance_count = volume.instances.size();
                 volume.gpu_dirty = false;
             }
             
             if(volume.instance_count > 0 && volume.instance_buffer) {
                 // Determine how many instances to draw based on Z clipping
                 // Binary search for the first instance that is ABOVe clipping Z
                 // Since instances are sorted by Z, we can just draw 0..Index
                 
                 GLsizei draw_count = (GLsizei)volume.instance_count;
                 
                 // Optimization: Only search if clipping is active (somewhere below max)
                 if (m_clipping_z < 10000.0f) {
                     // We find the first element that has Z > clipping_z + small margin
                     auto it = std::upper_bound(volume.instances.begin(), volume.instances.end(), m_clipping_z + 0.01f, 
                         [](float val, const Slic3r::ExtrusionGeometry::InstanceData& inst) {
                             return val < inst.posA[2];
                         });
                      draw_count = (GLsizei)std::distance(volume.instances.begin(), it);
                 }

                 if (draw_count > 0) {
                     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, volume.instance_buffer->get_id());
                     
                     m_vao_instanced_template->bind();
                     glDrawArraysInstanced(GL_TRIANGLES, 0, (GLsizei)m_vbo_instanced_template->get_count(), draw_count);
                     m_vao_instanced_template->unbind();

                     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0); 
                 }
             }
             
             // Restore Main Shader
             m_shader->bind();
        } else {
            // --- Standard Render Path ---
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
}

void Scene3D::draw_selection_box(const BoundingBoxf3& bb) {
    float size_x = bb.max.x - bb.min.x;
    float size_y = bb.max.y - bb.min.y;
    float size_z = bb.max.z - bb.min.z;
    
    // Calculate per-axis lengths
    // User requested: min 15%, max 30% of dimension to prevent touching on thin objects.
    auto get_len = [](float dim) {
        float l = dim * 0.15f; // Base: 15%
        
        // Absolute minimum for visibility, but allow it to be smaller for very tiny objects 
        // via the relative cap below.
        if (l < 1.0f) l = 1.0f; 
        
        // Strict relative cap at 30% ensures lines never touch (leaving at least 40% gap)
        if (l > dim * 0.3f) l = dim * 0.3f;
        
        return l;
    };

    float Lx = get_len(size_x);
    float Ly = get_len(size_y);
    float Lz = get_len(size_z);

    // Old safety clamp (50%) is superseded by the strict 30% cap above, 
    // but we keep the variables assignment clean.


    
    std::vector<float> v;
    auto add_corner = [&](float x, float y, float z, float dx, float dy, float dz) {
        v.insert(v.end(), {x, y, z, x + dx, y, z});
        v.insert(v.end(), {x, y, z, x, y + dy, z});
        v.insert(v.end(), {x, y, z, x, y, z + dz});
    };

    add_corner(bb.min.x, bb.min.y, bb.min.z,  Lx,  Ly,  Lz);
    add_corner(bb.max.x, bb.min.y, bb.min.z, -Lx,  Ly,  Lz);
    add_corner(bb.max.x, bb.max.y, bb.min.z, -Lx, -Ly,  Lz);
    add_corner(bb.min.x, bb.max.y, bb.min.z,  Lx, -Ly,  Lz);

    add_corner(bb.min.x, bb.min.y, bb.max.z,  Lx,  Ly, -Lz);
    add_corner(bb.max.x, bb.min.y, bb.max.z, -Lx,  Ly, -Lz);
    add_corner(bb.max.x, bb.max.y, bb.max.z, -Lx, -Ly, -Lz);
    add_corner(bb.min.x, bb.max.y, bb.max.z,  Lx, -Ly, -Lz);
    
    m_shader->set_uniform("u_lit", 0);
    m_shader->set_uniform("objectColor", glm::vec3(1.0f, 1.0f, 1.0f)); // White
    m_shader->set_uniform("u_alpha", 1.0f);
    
    m_vao_selection->bind();
    m_vbo_selection->upload_data(v.data(), v.size() * sizeof(float));
    m_vao_selection->add_attribute(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);
    
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, (GLsizei)(v.size() / 3));
    
    m_shader->set_uniform("u_lit", 1);
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
                grid_verts.push_back(0.0f);
            }
        }
        // Bed contours
        for(const Line &line : expoly.lines()){
            grid_verts.push_back(unscale(line.a.x));
            grid_verts.push_back(unscale(line.a.y));
            grid_verts.push_back(0.0f);
            grid_verts.push_back(unscale(line.b.x));
            grid_verts.push_back(unscale(line.b.y));
            grid_verts.push_back(0.0f);
        }
    }
    m_bed_dirty = true;
    Refresh();
}

bool Scene3D::mouse_move(wxMouseEvent &e){
    if (m_imgui && m_imgui->update_mouse_data(e)) {
        Refresh(); // Ensure UI updates when hovering
        return true;
    }
    if(e.Dragging()){
        const auto pos = Point(e.GetX(),e.GetY());
        if(dragging){
             if (e.LeftIsDown() && !m_prevent_camera_movement) {
                // Rotate (Inverted to match expected direction)
                float dx = (pos.x - drag_start.x);
                float dy = (pos.y - drag_start.y);
                m_camera.rotate(-dy * 0.8f, -dx * 0.8f);
            } else if (e.MiddleIsDown() || e.RightIsDown()) {
                // Pan
                float dx = (pos.x - drag_start.x);
                float dy = (pos.y - drag_start.y);
                m_camera.pan(dx, dy);
            }
            Refresh();
        }
        dragging = true;
        drag_start = pos; 
    }else{
        e.Skip();
    }
    return false;
}

bool Scene3D::mouse_up(wxMouseEvent &e){
    if (m_imgui && m_imgui->update_mouse_data(e)) {
        Refresh();
        return true;
    }
    dragging = false;
    Refresh();
    return false;
}

bool Scene3D::mouse_down(wxMouseEvent &e){
    this->SetFocus(); // Ensure canvas has focus for keyboard/imgui
    if (m_imgui && m_imgui->update_mouse_data(e)) {
        Refresh();
        return true;
    }
    dragging = false;
    drag_start = Point(e.GetX(), e.GetY());
    e.Skip();
    return false;
}

bool Scene3D::mouse_wheel(wxMouseEvent &e){
    if (m_imgui && m_imgui->update_mouse_data(e)) return true;
    float delta = ((float)e.GetWheelRotation()) / e.GetWheelDelta();
    // Increased zoom speed from 2% to 10% per notch for better responsiveness
    float zoom_factor = std::pow(1.1f, delta);
    m_camera.zoom(zoom_factor);
    Refresh();
    return false;
}

bool Scene3D::mouse_dclick(wxMouseEvent &e){
    // Reset view?
    Refresh();
    return false;
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
    m_shader->set_uniform("model", glm::mat4(1.0f)); 
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
    if (colors.solid_background) return;
    
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