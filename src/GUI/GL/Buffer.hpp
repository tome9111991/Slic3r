#ifndef SLIC3R_GUI_GL_BUFFER_HPP
#define SLIC3R_GUI_GL_BUFFER_HPP

#include "GLManager.hpp"
#include <vector>

namespace Slic3r {
namespace GUI {
namespace GL {

class VertexBuffer {
public:
    VertexBuffer();
    ~VertexBuffer();

    void bind();
    void unbind();
    
    // Upload data
    void upload_data(const void* data, size_t size, GLenum usage = GL_STATIC_DRAW);
    
    GLuint get_id() const { return m_id; }
    size_t get_count() const { return m_count; }
    void set_count(size_t count) { m_count = count; }

private:
    GLuint m_id = 0;
    size_t m_count = 0; 
};

class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    void bind();
    void unbind();
    
    // Link currently bound VBO to this VAO attribute
    void add_attribute(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);

private:
    GLuint m_id = 0;
};

} // namespace GL
} // namespace GUI
} // namespace Slic3r

#endif
