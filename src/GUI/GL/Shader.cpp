#include "Shader.hpp"
#include "libslic3r/Log.hpp"
#include <vector>
#include <glm/gtc/type_ptr.hpp>

namespace Slic3r {
namespace GUI {
namespace GL {

Shader::Shader(const std::string& name) : m_name(name) {}

Shader::~Shader() {
    if (m_id != 0) {
        glDeleteProgram(m_id);
    }
}

void Shader::bind() {
    if (m_id != 0) glUseProgram(m_id);
}

void Shader::unbind() {
    glUseProgram(0);
}

bool Shader::init(const std::string& vs_source, const std::string& fs_source) {
    GLuint vs, fs;
    
    // Vertex Shader
    const char* vs_code = vs_source.c_str();
    vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_code, NULL);
    glCompileShader(vs);
    if (!check_compile_errors(vs, "VERTEX")) return false;

    // Fragment Shader
    const char* fs_code = fs_source.c_str();
    fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_code, NULL);
    glCompileShader(fs);
    if (!check_compile_errors(fs, "FRAGMENT")) return false;

    // Shader Program
    m_id = glCreateProgram();
    glAttachShader(m_id, vs);
    glAttachShader(m_id, fs);
    glLinkProgram(m_id);
    if (!check_compile_errors(m_id, "PROGRAM")) return false;

    glDeleteShader(vs);
    glDeleteShader(fs);

    return true;
}

GLint Shader::get_uniform_location(const std::string& name) {
    if (m_uniform_cache.find(name) != m_uniform_cache.end())
        return m_uniform_cache[name];

    GLint location = glGetUniformLocation(m_id, name.c_str());
    if (location == -1) {
        // Slic3r::Log::warn("GUI", "Uniform '" + name + "' not found in shader '" + m_name + "'");
    }
    
    m_uniform_cache[name] = location;
    return location;
}

void Shader::set_uniform(const std::string& name, int value) {
    glUniform1i(get_uniform_location(name), value);
}

void Shader::set_uniform(const std::string& name, float value) {
    glUniform1f(get_uniform_location(name), value);
}

void Shader::set_uniform(const std::string& name, const glm::vec3& value) {
    glUniform3fv(get_uniform_location(name), 1, &value[0]);
}

void Shader::set_uniform(const std::string& name, const glm::vec4& value) {
    glUniform4fv(get_uniform_location(name), 1, &value[0]);
}

void Shader::set_uniform(const std::string& name, const glm::mat4& value) {
    glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, glm::value_ptr(value));
}

bool Shader::check_compile_errors(GLuint shader, std::string type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            Slic3r::Log::error("GUI", "Shader Compilation Error (" + m_name + ":" + type + "): " + infoLog);
            return false;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            Slic3r::Log::error("GUI", "Shader Linking Error (" + m_name + "): " + infoLog);
            return false;
        }
    }
    return true;
}

} // namespace GL
} // namespace GUI
} // namespace Slic3r
