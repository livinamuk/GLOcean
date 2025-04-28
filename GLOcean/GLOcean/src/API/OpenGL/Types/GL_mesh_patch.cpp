#include "GL_mesh_patch.h"

#include <utility>
#include <cassert>
#include <cstring>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

void OpenGLMeshPatch::CleanUp() {
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
    glDeleteVertexArrays(1, &m_VAO);
    m_VAO = 0;
    m_VBO = 0;
    m_EBO = 0;
    m_indexCount = 0;
}

void OpenGLMeshPatch::Resize(int width, int height) {
    m_vertices.resize(static_cast<size_t>(width) * height);
    m_width = width;
    m_height = height;

    CleanUp();
    CreateBuffers();
}

void OpenGLMeshPatch::Resize2(int width, int height) {
    m_vertices.resize(static_cast<size_t>(width) * height);
    m_width = width;
    m_height = height;

    CleanUp();
    CreateBuffers2();
}

void OpenGLMeshPatch::CreateBuffers() {
    if (m_width <= 0 || m_height <= 0) return;

    int n = m_width;
    int m = m_height;

    // Positions
    float N_half = (static_cast<float>(n) - 1.0f) * 0.5f;
    float M_half = (static_cast<float>(m) - 1.0f) * 0.5f;

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            m_vertices[i * n + j].position = glm::vec3(
                static_cast<float>(j),
                0.0f,
                static_cast<float>(i)
            );
            // Default normals
            m_vertices[i * n + j].normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    std::vector<unsigned int> indices;
    
    // Reserve an upper bound (approx)
    indices.reserve((n * (m - 1) * 2) + 2 * (m - 2));

    for (int i = 0; i < m - 1; ++i) {
        bool even = (i % 2) == 0;

        // build the vertical ladder for this row
        if (even) {
            for (int j = 0; j < n; ++j) {
                indices.push_back(i * n + j);
                indices.push_back((i + 1) * n + j);
            }
        }
        else {
            for (int j = n - 1; j >= 0; --j) {
                indices.push_back((i + 1) * n + j);
                indices.push_back(i * n + j);
            }
        }

        // insert two degenerates to restart without flipping
        if (i < m - 2) {

            // last vertex of this strip
            unsigned int last = indices.back();

            // first vertex of next strip
            unsigned int nextFirst = even
                ? ((i + 1) * n + (n - 1))   // even ended at bottom right
                : ((i + 1) * n + 0);        // odd  ended at top   left

            indices.push_back(last);
            indices.push_back(nextFirst);
        }
    }
    m_indexCount = 2 * n * (m - 1) + 2 * (m - 2);

    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(VertexPN), m_vertices.data(), GL_DYNAMIC_DRAW);

    glGenBuffers(1, &m_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    const GLsizei stride = sizeof(VertexPN);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(0);

    const void* normalOffsetPtr = reinterpret_cast<const void*>(offsetof(VertexPN, normal));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, normalOffsetPtr);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void OpenGLMeshPatch::CreateBuffers2() {
    if (m_width <= 0 || m_height <= 0) return;

    int n = m_width;
    int m = m_height;

    // Positions
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            m_vertices[i * n + j].position = glm::vec3(
                static_cast<float>(j),
                0.0f,
                static_cast<float>(i)
            );
            // Default normals
            m_vertices[i * n + j].normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    std::vector<unsigned int> indices;
    for (int y = 0; y < m_height - 1; ++y) {
        for (int x = 0; x < m_width - 1; ++x) {
            int topLeft = y * m_width + x;
            int topRight = topLeft + 1;
            int bottomLeft = (y + 1) * m_width + x;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
            indices.push_back(topRight);
        }
    }

    m_indexCount = indices.size();

    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(VertexPN), m_vertices.data(), GL_DYNAMIC_DRAW);

    glGenBuffers(1, &m_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    const GLsizei stride = sizeof(VertexPN);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(0);

    const void* normalOffsetPtr = reinterpret_cast<const void*>(offsetof(VertexPN, normal));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, normalOffsetPtr);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}