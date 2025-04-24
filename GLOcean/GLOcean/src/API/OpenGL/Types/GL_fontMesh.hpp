#pragma once
#include <vector>
#include <glad/glad.h>
#include "HellTypes.h"

struct OpenGLFontMesh {
private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    GLuint m_indexCount = 0;
    GLsizei m_vertexBufferSize = 0;
    GLsizei m_indexBufferSize = 0;

public:
    void UpdateVertexBuffer(std::vector<FontVertex>& vertices, std::vector<uint32_t>& indices) {
        m_indexCount = indices.size();
        if (m_indexCount == 0) {
            return;
        }
        GLsizei vertexBufferSize = vertices.size() * sizeof(FontVertex);
        GLsizei indexBufferSize = indices.size() * sizeof(uint32_t);
        if (vertexBufferSize > m_vertexBufferSize || indexBufferSize > m_indexBufferSize) {
            CleanUp();
            Create();
            m_vertexBufferSize = vertexBufferSize;
            m_indexBufferSize = indexBufferSize;
            glBindVertexArray(m_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, nullptr, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, nullptr, GL_DYNAMIC_DRAW);
        }
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertexBufferSize, vertices.data());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexBufferSize, indices.data());
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(FontVertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(FontVertex), (void*)offsetof(FontVertex, uv));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(FontVertex), (void*)offsetof(FontVertex, color));
        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    int GetVAO() {
        return m_vao;
    }

    int GetIndexCount() {
        return m_indexCount;
    }

    void Create() {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);
    }

    void CleanUp() {
        if (m_vao != 0) {
            glDeleteVertexArrays(1, &m_vao);
            glDeleteBuffers(1, &m_vbo);
            glDeleteBuffers(1, &m_ebo);
            m_vao = 0;
            m_vbo = 0;
            m_ebo = 0;
        }
    }
};