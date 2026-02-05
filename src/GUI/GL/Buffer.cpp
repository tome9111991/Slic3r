#include "Buffer.hpp"

namespace Slic3r {
namespace GUI {
namespace GL {

// VertexBuffer Implementation
VertexBuffer::VertexBuffer() {
    glGenBuffers(1, &m_id);
}

VertexBuffer::~VertexBuffer() {
    if (m_id != 0) glDeleteBuffers(1, &m_id);
}

void VertexBuffer::bind() {
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
}

void VertexBuffer::unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::upload_data(const void* data, size_t size, GLenum usage) {
    bind();
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
}

// VertexArray Implementation
VertexArray::VertexArray() {
    glGenVertexArrays(1, &m_id);
}

VertexArray::~VertexArray() {
    if (m_id != 0) glDeleteVertexArrays(1, &m_id);
}

void VertexArray::bind() {
    glBindVertexArray(m_id);
}

void VertexArray::unbind() {
    glBindVertexArray(0);
}

void VertexArray::add_attribute(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
    bind(); // Ensure VAO is bound
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

} // namespace GL
} // namespace GUI
} // namespace Slic3r
