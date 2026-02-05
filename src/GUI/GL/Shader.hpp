#ifndef SLIC3R_GUI_GL_SHADER_HPP
#define SLIC3R_GUI_GL_SHADER_HPP

#include "GLManager.hpp"
#include <string>
#include <glm/glm.hpp>
#include <unordered_map>

namespace Slic3r {
namespace GUI {
namespace GL {

class Shader {
public:
    Shader(const std::string& name);
    ~Shader();

    bool init(const std::string& vs_source, const std::string& fs_source);
    void bind();
    void unbind();
    
    void set_uniform(const std::string& name, int value);
    void set_uniform(const std::string& name, float value);
    void set_uniform(const std::string& name, const glm::vec3& value);
    void set_uniform(const std::string& name, const glm::vec4& value);
    void set_uniform(const std::string& name, const glm::mat4& value);

    GLuint get_id() const { return m_id; }

private:
    GLuint m_id = 0;
    std::string m_name;
    std::unordered_map<std::string, GLint> m_uniform_cache;

    GLint get_uniform_location(const std::string& name);
    bool check_compile_errors(GLuint shader, std::string type);
};

} // namespace GL
} // namespace GUI
} // namespace Slic3r

#endif
