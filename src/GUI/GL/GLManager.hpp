#ifndef SLIC3R_GUI_GL_GLMANAGER_HPP
#define SLIC3R_GUI_GL_GLMANAGER_HPP

#include <glad/glad.h>
#include <string>

namespace Slic3r {
namespace GUI {
namespace GL {

class GLManager {
public:
    static bool init();
    static bool is_initialized();
    static void check_error(const std::string& label = "");
private:
    static bool s_initialized;
};

} // namespace GL
} // namespace GUI
} // namespace Slic3r

#endif // SLIC3R_GUI_GL_GLMANAGER_HPP
