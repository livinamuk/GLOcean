#pragma once

#include <vector>
#include <cstddef>
#include <glad/glad.h>
#include <glm/vec3.hpp>
#include "HellTypes.h"

struct OpenGLMeshPatch {
    OpenGLMeshPatch() = default;
    OpenGLMeshPatch(size_t size);
    OpenGLMeshPatch(size_t width, size_t height);

    void resize(int width, int height);
    const VertexPN* data() const;
    void computeNormals();
    void refreshGPU();

    const size_t GetWidth() const         { return m_width; }
    const size_t GetHeight() const        { return m_height; }
    const GLuint GetVAO() const             { return m_VAO; }
    const GLuint GetVBO() const             { return m_VBO; }
    const GLuint GetEBO() const             { return m_EBO; }
    const GLsizei GetIndexCount() const     { return m_indexCount; }

private:
    void recreate();
    void CleanUp();

    std::vector<VertexPN> m_vertices;
    size_t m_width = 0;
    size_t m_height = 0;
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;
    GLsizei m_indexCount = 0;
};