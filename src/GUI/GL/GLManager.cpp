#include "GLManager.hpp"
#include "libslic3r/Log.hpp"

namespace Slic3r {
namespace GUI {
namespace GL {

bool GLManager::s_initialized = false;

bool GLManager::init() {
    if (s_initialized) return true;

    // In wxWidgets, the context must be current before loading
    // gladLoadGL uses the current context loader
    if (!gladLoadGL()) {
        Slic3r::Log::error("GUI", "Failed to initialize GLAD");
        return false;
    }
    
    Slic3r::Log::info("GUI", std::string("OpenGL Initialized: ") + (const char*)glGetString(GL_VERSION));
    Slic3r::Log::info("GUI", std::string("Renderer: ") + (const char*)glGetString(GL_RENDERER));
    
    s_initialized = true;
    return true;
}

bool GLManager::is_initialized() {
    return s_initialized;
}

void GLManager::check_error(const std::string& label) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        Slic3r::Log::error("GUI", "OpenGL Error [" + label + "]: " + std::to_string(err));
    }
}

} // namespace GL
} // namespace GUI
} // namespace Slic3r
