#include "Scene3D.hpp"
#include "Theme/CanvasTheme.hpp"
#include "Line.hpp"
#include "ClipperUtils.hpp"
#include "misc_ui.hpp"
#include "Log.hpp"
#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
namespace Slic3r { namespace GUI {

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

#ifdef _WIN32
#include <windows.h>
#endif

// OpenGL Function Pointers
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31

typedef void (APIENTRY *PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRY *PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRY *PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC) (void);
typedef void (APIENTRY *PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRY *PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRY *PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLint (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRY *PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);
typedef void (APIENTRY *PFNGLUNIFORM3FPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRY *PFNGLDISABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef GLint (APIENTRY *PFNGLGETATTRIBLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRY *PFNGLVERTEXATTRIB1FPROC) (GLuint index, GLfloat x);

static PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
static PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
static PFNGLCREATESHADERPROC glCreateShader = nullptr;
static PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
static PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
static PFNGLATTACHSHADERPROC glAttachShader = nullptr;
static PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
static PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
static PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
static PFNGLUNIFORM1FPROC glUniform1f = nullptr;
static PFNGLUNIFORM3FPROC glUniform3f = nullptr;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = nullptr;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
static PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation = nullptr;
static PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f = nullptr;

static bool extensions_loaded = false;

static void load_gl_extensions() {
    if (extensions_loaded) return;
    
    #ifdef _WIN32
    glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
    glUniform3f = (PFNGLUNIFORM3FPROC)wglGetProcAddress("glUniform3f");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
    glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
    glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation");
    glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)wglGetProcAddress("glVertexAttrib1f");
    
    if (!glUseProgram) {
        Slic3r::Log::error("GUI", "Failed to load OpenGL extensions.");
    } else {
        Slic3r::Log::info("GUI", "OpenGL extensions loaded successfully.");
        extensions_loaded = true;
    }
    #endif
}

// Define attributes as a static array to avoid C4576 (non-standard C++ extension)
static const int gl_attrs[] = {
    WX_GL_RGBA, 
    WX_GL_DOUBLEBUFFER, 
    WX_GL_DEPTH_SIZE, 16,
    WX_GL_SAMPLE_BUFFERS, 1,
    WX_GL_SAMPLES, 4,
    0
};

Scene3D::Scene3D(wxWindow* parent, const wxSize& size) :
    wxGLCanvas(parent, wxID_ANY, gl_attrs, wxDefaultPosition, size)
{ 


    wxGLContextAttrs ctxAttrs;
    ctxAttrs.PlatformDefaults().OGLVersion(4, 6).CompatibilityProfile().EndList();
    this->glContext = new wxGLContext(this, nullptr, &ctxAttrs);
    this->Bind(wxEVT_PAINT, [this](wxPaintEvent &e) { this->repaint(e); });
    this->Bind(wxEVT_SIZE, [this](wxSizeEvent &e ){
        dirty = true;
        Refresh();
    });
    
    
    // Bind the varying mouse events
    this->Bind(wxEVT_MOTION, [this](wxMouseEvent &e) { this->mouse_move(e); });
    this->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent &e) { this->mouse_up(e); });
    this->Bind(wxEVT_RIGHT_UP, [this](wxMouseEvent &e) { this->mouse_up(e); });
    this->Bind(wxEVT_MIDDLE_DCLICK, [this](wxMouseEvent &e) { this->mouse_dclick(e); });
    this->Bind(wxEVT_MOUSEWHEEL, [this](wxMouseEvent &e) { this->mouse_wheel(e); });
    
    Points p;
    const coord_t w = scale_(200), z = 0;
    p.push_back(Point(z,z));
    p.push_back(Point(z,w));
    p.push_back(Point(w,w));
    p.push_back(Point(w,z));
    set_bed_shape(p);
}

float clamp(float low, float x, float high){
    if(x < low) return low;
    if(x > high) return high;
    return x;
}

Linef3 Scene3D::mouse_ray(Point win){
    GLdouble proj[16], mview[16]; 
    glGetDoublev(GL_MODELVIEW_MATRIX, mview);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    GLint view[4];
    glGetIntegerv(GL_VIEWPORT, view);
    win.y = view[3]-win.y;
    GLdouble x = 0.0, y = 0.0, z = 0.0;
    gluUnProject(win.x,win.y,0,mview,proj,view,&x,&y,&z);
    Pointf3 first = Pointf3(x,y,z);
    GLint a = gluUnProject(win.x,win.y,1,mview,proj,view,&x,&y,&z);
    return Linef3(first,Pointf3(x,y,z)); 
}

void Scene3D::mouse_move(wxMouseEvent &e){
    if(e.Dragging()){
        //const auto s = GetSize();
        const auto pos = Point(e.GetX(),e.GetY());
        if(dragging){
            if (e.ShiftDown()) { // TODO: confirm alt -> shift is ok
                // Move the camera center on the Z axis based on mouse Y axis movement
                _camera_target.translate(0, 0, (pos.y - drag_start.y));
            } else if (e.LeftIsDown()) {
                // if dragging over blank area with left button, rotate
                //if (TURNTABLE_MODE) {
                const float TRACKBALLSIZE = 0.8f, GIMBAL_LOCK_THETA_MAX = 170.0f;
                                
                phi += (pos.x - drag_start.x) * TRACKBALLSIZE;
                theta -= (pos.y - drag_start.y) * TRACKBALLSIZE;
                theta = clamp(0, theta, GIMBAL_LOCK_THETA_MAX);
                /*} else {
                    my $size = $self->GetClientSize;
                    my @quat = trackball(
                        $orig->x / ($size->width / 2) - 1,
                        1 - $orig->y / ($size->height / 2),       #/
                        $pos->x / ($size->width / 2) - 1,
                        1 - $pos->y / ($size->height / 2),        #/
                    );
                    $self->_quat(mulquats($self->_quat, \@quat));
                }*/
            } else if (e.MiddleIsDown() || e.RightIsDown()) {
                // if dragging over blank area with right button, translate
                // get point in model space at Z = 0
                const auto current = mouse_ray(pos).intersect_plane(0);
                const auto old = mouse_ray(drag_start).intersect_plane(0);
                _camera_target.translate(current.vector_to(old));
            }
            //$self->on_viewport_changed->() if $self->on_viewport_changed;
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
        // Calculate the zoom delta and apply it to the current zoom factor
        auto _zoom = ((float)e.GetWheelRotation()) / e.GetWheelDelta();
        /*if ($Slic3r::GUI::Settings->{_}{invert_zoom}) {
            _zoom *= -1;
        }*/
        _zoom = clamp(-4, _zoom,4);
        _zoom /= 10;
        zoom /= 1-_zoom;
        /* 
        # In order to zoom around the mouse point we need to translate
        # the camera target
        my $size = Slic3r::Pointf->new($self->GetSizeWH);
        my $pos = Slic3r::Pointf->new($e->GetX, $size->y - $e->GetY); #-
        $self->_camera_target->translate(
            # ($pos - $size/2) represents the vector from the viewport center
            # to the mouse point. By multiplying it by $zoom we get the new,
            # transformed, length of such vector.
            # Since we want that point to stay fixed, we move our camera target
            # in the opposite direction by the delta of the length of such vector
            # ($zoom - 1). We then scale everything by 1/$self->_zoom since 
            # $self->_camera_target is expressed in terms of model units.
            -($pos->x - $size->x/2) * ($zoom) / $self->_zoom,
            -($pos->y - $size->y/2) * ($zoom) / $self->_zoom,
            0,
        ) if 0;
        */

        dirty = true;
        Refresh();
}

void Scene3D::mouse_dclick(wxMouseEvent &e){
    /*
    if (@{$self->volumes}) {
        $self->zoom_to_volumes;
    } else {
        $self->zoom_to_bed;
    }*/
    
    dirty = true;
    Refresh();
}

void Scene3D::resize(){
    if(!dirty)return;
    dirty = false;
    const auto s = GetSize();
    glViewport(0,0,s.GetWidth(),s.GetHeight());
    const auto x = s.GetWidth()/zoom,
               y = s.GetHeight()/zoom,
               depth = 1000.0f; // my $depth = 10 * max(@{ $self->max_bounding_box->size });
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(
        -x/2, x/2, -y/2, y/2,
        -depth, 2*depth
    );
    glMatrixMode(GL_MODELVIEW);
}

void Scene3D::set_bed_shape(Points _bed_shape){
    
    bed_shape = _bed_shape;
    const float GROUND_Z = -0.02f;
    
    // triangulate bed
    const auto expoly = ExPolygon(Polygon(bed_shape));
    const auto box = expoly.bounding_box();
    bed_bound = box;
    
    // Update camera target to be the center of the bed
    Point center = bed_bound.center();
    _camera_target.x = unscale(center.x);
    _camera_target.y = unscale(center.y);
    _camera_target.z = 0.0f;

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
    {
        std::vector<Polyline> lines;
        Points tmp;
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
        // clip with a slightly grown expolygon because our lines lay on the contours and
        // may get erroneously clipped
        // my @lines = map Slic3r::Line->new(@$_[0,-1]),
        grid_verts.clear();
        const Polylines clipped = intersection_pl(lines,offset_ex(expoly,SCALED_EPSILON).at(0));
        for(const Polyline &line : clipped){
            for(const Point &point : line.points){
                grid_verts.push_back(unscale(point.x));
                grid_verts.push_back(unscale(point.y));
                grid_verts.push_back(GROUND_Z);
            }
        }
        // append bed contours
        for(const Line &line : expoly.lines()){
            grid_verts.push_back(unscale(line.a.x));
            grid_verts.push_back(unscale(line.a.y));
            grid_verts.push_back(GROUND_Z);
            grid_verts.push_back(unscale(line.b.x));
            grid_verts.push_back(unscale(line.b.y));
            grid_verts.push_back(GROUND_Z);
        }
    }
    
    //$self->origin(Slic3r::Pointf->new(0,0));
}

void Scene3D::init_gl(){
    if(this->init)return;
    this->init = true;

    if (const GLubyte* version = glGetString(GL_VERSION)) {
        Slic3r::Log::info("GUI", std::string("OpenGL Version: ") + (const char*)version);
    }

    glClearColor(0, 0, 0, 1);
    glColor3f(1, 0, 0);
    glEnable(GL_DEPTH_TEST);
    glClearDepth(1.0);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Fix normals after scaling
    glEnable(GL_NORMALIZE);

    // Set antialiasing/multisampling
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_MULTISAMPLE);
    
    // ambient lighting - Soft ambient
    GLfloat ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    
    // light from camera - Warmer key light
    GLfloat pos[] = {1.0f, 0.0f, 1.0f, 0.0f};
    GLfloat spec[] = {0.6f, 0.6f, 0.6f, 1.0f};
    GLfloat diff[] = {0.7f, 0.7f, 0.7f, 1.0f};
    glLightfv(GL_LIGHT1, GL_POSITION, pos);
    glLightfv(GL_LIGHT1, GL_SPECULAR, spec);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  diff);
    
    // Enables Smooth Color Shading
    glShadeModel(GL_SMOOTH);
    
    // Material settings - brighter, less plastic
    GLfloat fbdiff[] = {0.6f, 0.6f, 0.6f, 1.0f};
    GLfloat fbspec[] = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat fbemis[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, fbdiff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, fbspec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 60);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, fbemis);
    
    // A handy trick -- have surface material mirror the color.
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    //glEnable(GL_MULTISAMPLE) if ($self->{can_multisample});
    
    load_gl_extensions();
    if(extensions_loaded) {
        init_shaders();
    }
}

void Scene3D::init_shaders() {
    if (m_shader_program != 0) return;
    
    const char* vertex_src = R"(
#version 120
varying vec3 v_frag_pos;
varying vec3 v_normal;
varying vec4 v_color;

void main() {
    v_frag_pos = vec3(gl_ModelViewMatrix * gl_Vertex);
    v_normal = gl_NormalMatrix * gl_Normal;
    v_color = gl_Color;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
)";

    const char* fragment_src = R"(
#version 120
varying vec3 v_frag_pos;
varying vec3 v_normal;
varying vec4 v_color;

uniform float u_clipping_z;
uniform vec3 u_light_pos;

void main() {
    // Clipping (Check Z in world space? No, u_clipping_z is likely world space Z. 
    // We need world space pos in vertex shader to do this strictly correct, 
    // but typically simple clipping is done in view space or passed world pos.
    // Let's assume we can get world Z relatively easily.)
    // Wait, gl_Vertex in VS is object space. ModelView puts it in View Space.
    // To clip by "Z height" of the print, we usually mean Object Space Z (since model is usually at 0,0,0).
    // Let's pass object space Z to FS.
    // Re-writing Vertex Shader part below implicitly for clarity of thought:
    // v_obj_pos = gl_Vertex;
    
    // NOTE: We need to update the Vertex shader to pass object position.
}
)";

    // CORRECTED SHADERS with Tube Profile
    const char* v_src = R"(
#version 120
attribute float a_tube_x;
varying vec3 v_frag_pos; // View space
varying vec3 v_normal;   // View space
varying vec4 v_color;
varying float v_z_height; // Object space Z
varying float v_tube_x;

void main() {
    v_frag_pos = vec3(gl_ModelViewMatrix * gl_Vertex);
    v_normal = gl_NormalMatrix * gl_Normal;
    v_color = gl_Color;
    v_z_height = gl_Vertex.z; // Object space Z is what we clip against
    v_tube_x = a_tube_x;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
)";

    const char* f_src = R"(
#version 120
// Enable derivatives
#extension GL_ARB_fragment_shader_derivative : enable

varying vec3 v_frag_pos;
varying vec3 v_normal;
varying vec4 v_color;
varying float v_z_height;
varying float v_tube_x;

uniform float u_clipping_z;

void main() {
    if (v_z_height > u_clipping_z) discard;

    vec3 orig_norm = normalize(v_normal);
    vec3 N = orig_norm;
    
    // Auto-calculate Tube Normal using derivatives
    // We need 'tubeGradient' = direction in view space corresponding to increasing v_tube_x
    // dFdx return change per pixel X.
    
    vec3 dPosdx = dFdx(v_frag_pos);
    vec3 dPosdy = dFdy(v_frag_pos);
    
    // We can't trust normal from cross product of dPos if geometric normal is flat.
    // But we CAN calculate the gradient of v_tube_x in screen space.
    float dXdx = dFdx(v_tube_x);
    float dXdy = dFdy(v_tube_x);
    
    // Gradient vector in View Space:
    // This part is tricky. 
    // Simplified: We assume surface is flat.
    // 'tangent' is the direction on the surface where v_tube_x increases.
    // We can solve this with a system of equations but let's approximate.
    // gradient = (dPosdx * dXdx + dPosdy * dXdy) / (dXdx^2 + dXdy^2) ... no.
    
    // Let's use the cheap trick: Darken edges.
    // Authentic layers also cast shadows on each other (Ambient Occlusion).
    // The darkening of edges matches AO.
    
    // Improved Normal Mapping:
    // If v_tube_x is -1..1 across the width.
    // We want N to tilt.
    // The tilt direction should be 'across' the tube.
    // 'across' is perpendicular to 'along'.
    // If we assume the tube is roughly horizontal (Top View),
    // We can just guess the cross direction.
    
    // Let's fall back to a robust derivative approach if possible.
    // tube_dir = normalize( dPosdx * dXdx + dPosdy * dXdy ); // This is direction of change
    
    float sigma = dXdx*dXdx + dXdy*dXdy;
    if (sigma > 0.000001) {
        vec3 T = normalize( dPosdx * dXdx + dPosdy * dXdy ); // Vector pointing towards +1
        // T is View Space vector of increasing X.
        // We want N to be Mix(FaceNormal, T).
        // At v_tube_x = 0, N = FaceNormal.
        // At v_tube_x = 1, N = T (roughly).
        
    // Circular profile:
        // x = v_tube_x (sin theta)
        // z = sqrt(1-x^2) (cos theta)
        // Normal = FaceNormal * z + T * x
        
        float x = clamp(v_tube_x, -0.99, 0.99); // Clamp to avoid NaN
        float z = sqrt(1.0 - x*x);
        
    // Anti-aliasing / LOD for Normal Mapping
        // If the frequency of v_tube_x is too high (zoomed out), fade effect.
        float delta = fwidth(v_tube_x);
        float lod_fade = 1.0 - smoothstep(0.8, 1.5, delta);
        
        // Blend between Geometric Normal (flat) and Tube Normal based on LOD
        vec3 tube_normal = normalize(orig_norm * z + T * x);
        N = normalize(mix(orig_norm, tube_normal, lod_fade));
    }

    vec3 light_dir = normalize(vec3(0.5, 0.5, 1.0)); // Light from top-right-front
    
    // Material Properties - Matte Finish
    float shininess = 12.0; // Broad, soft highlight
    float ambientStrength = 0.4; // Reduced to prevent "glow"
    float specularStrength = 0.1; // Very low specular
    
    // Gamma Correction: Convert input color to Linear Space
    vec3 albedo = pow(v_color.rgb, vec3(2.2));
    
    // Ambient
    vec3 ambient = ambientStrength * vec3(1.0);
    
    // Diffuse
    float diff = max(dot(N, light_dir), 0.0);
    vec3 diffuse = diff * vec3(1.0);
    
    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(-v_frag_pos);
    vec3 halfwayDir = normalize(light_dir + viewDir);
    float spec = pow(max(dot(N, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * vec3(1.0);
    
    vec3 lighting = (ambient + diffuse + specular) * albedo;
    
    // Edge darkening/AO effect
    // Stronger strength (0.6) for deep grooves
    float ao_base = 1.0 - (v_tube_x * v_tube_x) * 0.6; 
    
    // Anti-aliasing for AO
    // Fade AO later to keep layers visible when zoomed out
    float delta_ao = fwidth(v_tube_x);
    // Push the fade threshold higher to keep texture at distance
    float ao_fade = 1.0 - smoothstep(0.8, 1.5, delta_ao);
    float ao = mix(1.0, ao_base, ao_fade); // mix(no_ao, full_ao, fade_factor)
    
    lighting *= ao;

    // Gamma Correction: Back to sRGB
    lighting = pow(lighting, vec3(1.0/2.2));

    gl_FragColor = vec4(lighting, v_color.a);
}
)";
    
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &v_src, NULL);
    glCompileShader(vs);
    
    GLint success;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        Slic3r::Log::error("GUI", std::string("Vertex Shader Compilation Failed: ") + infoLog);
    }
    
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &f_src, NULL);
    glCompileShader(fs);
    
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
        Slic3r::Log::error("GUI", std::string("Fragment Shader Compilation Failed: ") + infoLog);
    }
    
    m_shader_program = glCreateProgram();
    glAttachShader(m_shader_program, vs);
    glAttachShader(m_shader_program, fs);
    glLinkProgram(m_shader_program);
    
    glGetProgramiv(m_shader_program, GL_LINK_STATUS, &success);
    if (!success) {
         char infoLog[512];
         glGetProgramInfoLog(m_shader_program, 512, NULL, infoLog);
         Slic3r::Log::error("GUI", std::string("Shader Linking Failed: ") + infoLog);
    }
    
    m_u_clipping_z = glGetUniformLocation(m_shader_program, "u_clipping_z");
    m_a_tube_x = glGetAttribLocation(m_shader_program, "a_tube_x");
}


void Scene3D::draw_background(){
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    glBegin(GL_QUADS);
    auto colors = CanvasTheme::GetColors();
    auto bottom = colors.canvas_bg_bottom, top = colors.canvas_bg_top;
    if(colors.solid_background){
         bottom = top = colors.canvas_bg_top;
    }
    glColor3ub(bottom.Red(), bottom.Green(), bottom.Blue());
    glVertex2f(-1.0,-1.0);
    glVertex2f(1,-1.0);
    glColor3ub(top.Red(), top.Green(), top.Blue());
    glVertex2f(1, 1);
    glVertex2f(-1.0, 1);
    glEnd();
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void Scene3D::draw_ground(){
    glDisable(GL_DEPTH_TEST);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    /*my $triangle_vertex;
    if (HAS_VBO) {
        ($triangle_vertex) =
            glGenBuffersARB_p(1);
        $self->bed_triangles->bind($triangle_vertex);
        glBufferDataARB_p(GL_ARRAY_BUFFER_ARB, $self->bed_triangles, GL_STATIC_DRAW_ARB);
        glVertexPointer_c(3, GL_FLOAT, 0, 0);
    } else {*/
    // fall back on old behavior
    glVertexPointer(3, GL_FLOAT, 0, bed_verts.data());
    auto colors = CanvasTheme::GetColors();
    const auto ground = colors.ground_color, grid = colors.grid_color;
        
    glColor4ub(ground.Red(), ground.Green(), ground.Blue(),ground.Alpha());
    glNormal3d(0,0,1);
    glDrawArrays(GL_TRIANGLES, 0, bed_verts.size() / 3);
    
    // we need depth test for grid, otherwise it would disappear when looking
    // the object from below
    glEnable(GL_DEPTH_TEST);

    // draw grid
    glLineWidth(2);
    /*my $grid_vertex;
    if (HAS_VBO) {
        ($grid_vertex) =
            glGenBuffersARB_p(1);
        $self->bed_grid_lines->bind($grid_vertex);
        glBufferDataARB_p(GL_ARRAY_BUFFER_ARB, $self->bed_grid_lines, GL_STATIC_DRAW_ARB);
        glVertexPointer_c(3, GL_FLOAT, 0, 0);
    } else {*/
    // fall back on old behavior
    glVertexPointer(3, GL_FLOAT, 0, grid_verts.data());
    
    glColor4ub(grid.Red(), grid.Green(), grid.Blue(),grid.Alpha());
    glNormal3d(0,0,1);
    glDrawArrays(GL_LINES, 0, grid_verts.size() / 3);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glDisable(GL_BLEND);
    /*if (HAS_VBO) { 
        # Turn off buffer objects to let the rest of the draw code work.
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
        glDeleteBuffersARB_p($grid_vertex);
        glDeleteBuffersARB_p($triangle_vertex);
    }*/
}

void Scene3D::draw_axes (Pointf3 center, float length, int width, bool always_visible){
    /*
    my $volumes_bb = $self->volumes_bounding_box;
    
    {
        # draw axes
        # disable depth testing so that axes are not covered by ground
        glDisable(GL_DEPTH_TEST);
        my $origin = $self->origin;
        my $axis_len = max(
            max(@{ $self->bed_bounding_box->size }),
            1.2 * max(@{ $volumes_bb->size }),
        );
        glLineWidth(2);
        glBegin(GL_LINES);
        # draw line for x axis
        glColor3f(1, 0, 0);
        glVertex3f(@$origin, $ground_z);
        glVertex3f($origin->x + $axis_len, $origin->y, $ground_z);  #,,
        # draw line for y axis
        glColor3f(0, 1, 0);
        glVertex3f(@$origin, $ground_z);
        glVertex3f($origin->x, $origin->y + $axis_len, $ground_z);  #++
        glEnd();
        # draw line for Z axis
        # (re-enable depth test so that axis is correctly shown when objects are behind it)
        glEnable(GL_DEPTH_TEST);
        glBegin(GL_LINES);
        glColor3f(0, 0, 1);
        glVertex3f(@$origin, $ground_z);
        glVertex3f(@$origin, $ground_z+$axis_len);
        glEnd();
    }
   */ 
    if (always_visible) {
        glDisable(GL_DEPTH_TEST);
    } else {
        glEnable(GL_DEPTH_TEST);
    }
    glLineWidth(width);
    glBegin(GL_LINES);
    // draw line for x axis
    glColor3f(1, 0, 0);
    glVertex3f(center.x, center.y, center.z);
    glVertex3f(center.x + length, center.y, center.z);
    // draw line for y axis
    glColor3f(0, 1, 0);
    glVertex3f(center.x, center.y, center.z);
    glVertex3f(center.x, center.y + length, center.z);
    glEnd();

    // draw line for Z axis
    // (re-enable depth test so that axis is correctly shown when objects are behind it)
    glEnable(GL_DEPTH_TEST);
    glBegin(GL_LINES);
    glColor3f(0, 0, 1);
    glVertex3f(center.x, center.y, center.z);
    glVertex3f(center.x, center.y, center.z + length);
    glEnd();
}
void Scene3D::draw_volumes(){
    if (extensions_loaded && m_shader_program != 0) {
        glUseProgram(m_shader_program);
        if (m_u_clipping_z != -1) {
            glUniform1f(m_u_clipping_z, m_clipping_z);
        }
    } else {
        // Fallback for no shader support? Or just rely on fixed function
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    if (m_a_tube_x != -1) glEnableVertexAttribArray(m_a_tube_x);
    
    for(const Volume &volume : volumes){
        glPushMatrix();
        glTranslatef(volume.origin.x, volume.origin.y, volume.origin.z);
        glCullFace(GL_BACK);
        glVertexPointer(3, GL_FLOAT, 0, volume.model.verts.data());
        glNormalPointer(GL_FLOAT, 0, volume.model.norms.data());
        if (m_a_tube_x != -1 && !volume.tube_coords.empty()) {
            glVertexAttribPointer(m_a_tube_x, 1, GL_FLOAT, GL_FALSE, 0, volume.tube_coords.data());
        } else if (m_a_tube_x != -1) {
             // Disable or set default?
             glVertexAttrib1f(m_a_tube_x, 0.0f);
        }
        glColor4ub(volume.color.Red(), volume.color.Green(), volume.color.Blue(), 255);
        glDrawArrays(GL_TRIANGLES, 0, volume.model.verts.size()/3);

        if (volume.selected) {
            glDisable(GL_LIGHTING);
            glLineWidth(2.0f);
            glColor3f(1.0f, 1.0f, 1.0f);
            
            const Pointf3& min = volume.bb.min;
            const Pointf3& max = volume.bb.max;
            
            // Calculate corner line length (min of 10.0f or 20% of dimension)
            float lx = std::min(10.0f, (float)((max.x - min.x) * 0.2));
            float ly = std::min(10.0f, (float)((max.y - min.y) * 0.2));
            float lz = std::min(10.0f, (float)((max.z - min.z) * 0.2));

            glBegin(GL_LINES);
            for (float x : {min.x, max.x}) {
                for (float y : {min.y, max.y}) {
                    for (float z : {min.z, max.z}) {
                        float dx = (x == min.x) ? lx : -lx;
                        float dy = (y == min.y) ? ly : -ly;
                        float dz = (z == min.z) ? lz : -lz;
                        
                        // Line along X
                        glVertex3f(x, y, z); glVertex3f(x + dx, y, z);
                        // Line along Y
                        glVertex3f(x, y, z); glVertex3f(x, y + dy, z);
                        // Line along Z
                        glVertex3f(x, y, z); glVertex3f(x, y, z + dz);
                    }
                }
            }
            glEnd();
            glEnable(GL_LIGHTING);
        }

        glPopMatrix();
    }
    
    if (m_a_tube_x != -1) glDisableVertexAttribArray(m_a_tube_x);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
    
    if (extensions_loaded && m_shader_program != 0) {
        glUseProgram(0);
    }
}

void Scene3D::set_camera_view(Direction dir) {
    switch(dir) {
        case Direction::Top:      theta = 0.0f;   phi = 0.0f;   break;
        case Direction::Bottom:   theta = 180.0f; phi = 0.0f;   break;
        case Direction::Front:    theta = 90.0f;  phi = 0.0f;   break;
        case Direction::Back:     theta = 90.0f;  phi = 180.0f; break;
        case Direction::Left:     theta = 90.0f;  phi = -90.0f; break;
        case Direction::Right:    theta = 90.0f;  phi = 90.0f;  break;
        case Direction::Diagonal: theta = 45.0f;  phi = 45.0f;  break;
    }
    dirty = true;
    Refresh();
}

void Scene3D::repaint(wxPaintEvent& e) {
    if(!this->IsShownOnScreen())return;
    // There should be a context->IsOk check once wx is updated
    if(!this->SetCurrent(*(this->glContext)))return;
    init_gl();
    resize();

    glClearColor(1, 1, 1, 1);
    glClearDepth(1);
    glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glRotatef(-theta, 1, 0, 0); // pitch
    glRotatef(phi, 0, 0, 1);   // yaw
    /*} else {
        my @rotmat = quat_to_rotmatrix($self->quat);
        glMultMatrixd_p(@rotmat[0..15]);
    }*/
    glTranslatef(-_camera_target.x, -_camera_target.y, -_camera_target.z);
     
    // light from above
    GLfloat pos[] = {-0.5f, -0.5f, 1.0f, 0.0f}, spec[] = {0.2f, 0.2f, 0.2f, 1.0f}, diff[] = {0.5f, 0.5f, 0.5f, 1.0f};    
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diff);

    before_render();

    draw_background();
    draw_ground();
        /*my $origin = $self->origin;
        my $axis_len = max(
            max(@{ $self->bed_bounding_box->size }),
            1.2 * max(@{ $volumes_bb->size }),
        );*/
    draw_axes(Pointf3(0.0f,0.0f,0.0f), 20.0f, 2, true);
    
    // draw objects
    glEnable(GL_LIGHTING);
    draw_volumes();
    
    after_render();
    
    if (dragging/*defined $self->_drag_start_pos || defined $self->_drag_start_xy*/) {
        draw_axes(_camera_target, 10.0f, 1, true/*camera,10,1,true*/);
        draw_axes(_camera_target, 10.0f, 4, false/*camera,10,4,false*/);
    }

    glFlush();
    SwapBuffers();
    // Calling glFinish has a performance penalty, but it seems to fix some OpenGL driver hang-up with extremely large scenes.
    glFinish();

}

} } // Namespace Slic3r::GUI
