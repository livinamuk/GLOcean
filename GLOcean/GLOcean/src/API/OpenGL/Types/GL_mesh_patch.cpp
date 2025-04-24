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

OpenGLMeshPatch::OpenGLMeshPatch(size_t size)
    : OpenGLMeshPatch(size, size)
{
}

OpenGLMeshPatch::OpenGLMeshPatch(size_t width, size_t height)
    : m_vertices(static_cast<size_t>(width)* height),
    m_width(width),
    m_height(height)
{
    assert(width > 0 && height > 0);
    recreate(); // Create vertices and GPU resources
}


void OpenGLMeshPatch::resize(int width, int height) {
    assert(width > 0 && height > 0);

    CleanUp();

    m_vertices.resize(static_cast<size_t>(width) * height);
    m_width = width;
    m_height = height;

    recreate();
}

const VertexPN* OpenGLMeshPatch::data() const {
    return !m_vertices.empty() ? m_vertices.data() : nullptr;
}

void OpenGLMeshPatch::computeNormals() {
    if (m_width <= 0 || m_height <= 0) return;

    for (int i = 0; i < m_height; ++i) {
        for (int j = 0; j < m_width; ++j) {
            const int index = i * m_width + j;

            const glm::vec3& currentPos = m_vertices[index].mPos;
            const glm::vec3& leftPos = (j == 0) ? currentPos : m_vertices[index - 1].mPos;
            const glm::vec3& rightPos = (j == m_width - 1) ? currentPos : m_vertices[index + 1].mPos;
            const glm::vec3& topPos = (i == 0) ? currentPos : m_vertices[index - m_width].mPos;
            const glm::vec3& bottomPos = (i == m_height - 1) ? currentPos : m_vertices[index + m_width].mPos;

            glm::vec3 verticalVec = bottomPos - topPos;
            glm::vec3 horizontalVec = rightPos - leftPos;

            if (glm::length(verticalVec) < 1e-6f || glm::length(horizontalVec) < 1e-6f || glm::length(glm::cross(verticalVec, horizontalVec)) < 1e-6f) {
                m_vertices[index].mNormal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
            else {
                m_vertices[index].mNormal = glm::normalize(glm::cross(verticalVec, horizontalVec));
            }
        }
    }
}

void OpenGLMeshPatch::refreshGPU() {
    if (m_VBO == 0 || m_vertices.empty()) return; // No buffer or no data

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0, // Offset
                    m_vertices.size() * sizeof(VertexPN), // Size
                    m_vertices.data()); // Data pointer
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind
}

void OpenGLMeshPatch::recreate() {
    if (m_width <= 0 || m_height <= 0) return;

    int n = m_width;
    int m = m_height;

    // Positions
    float N_half = (static_cast<float>(n) - 1.0f) * 0.5f;
    float M_half = (static_cast<float>(m) - 1.0f) * 0.5f;

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            m_vertices[i * n + j].mPos = glm::vec3(
                static_cast<float>(j) - N_half,
                0.0f,
                static_cast<float>(i) - M_half
            );
            // Default normals
            m_vertices[i * n + j].mNormal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    // Compute Normals based on positions
    computeNormals();

    std::vector<unsigned int> indices;
// reserve an upper bound (approx)
    indices.reserve((n * (m - 1) * 2) + 2 * (m - 2));

    for (int i = 0; i < m - 1; ++i) {
        bool even = (i % 2) == 0;
        // build the vertical “ladder” for this row
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
                : ((i + 1) * n + 0);      // odd  ended at top   left
            indices.push_back(last);
            indices.push_back(nextFirst);
        }
    }
    m_indexCount = 2 * n * (m - 1) + 2 * (m - 2);

    // Generate and Bind VAO
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    // Generate, Bind, and Buffer VBO (Vertex Buffer)
    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 m_vertices.size() * sizeof(VertexPN),
                 m_vertices.data(),
                 GL_DYNAMIC_DRAW); // Usage hint (can be GL_STATIC_DRAW if vertices don't change often)

    // Generate, Bind, and Buffer EBO (Index Buffer)
    glGenBuffers(1, &m_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO); // EBO binding *is* part of VAO state
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW); // Indices usually don't change

    // --- 5. Set Vertex Attribute Pointers (part of VAO state) ---
    const GLsizei stride = sizeof(VertexPN);

    // Position attribute (location = 0)
    glVertexAttribPointer(0, // Attribute index 0
                          3, // Number of components (x, y, z)
                          GL_FLOAT, // Type of components
                          GL_FALSE, // Normalized?
                          stride,   // Stride (bytes between consecutive vertices)
                          nullptr); // Offset of first component
    glEnableVertexAttribArray(0);   // Enable attribute 0

    // Normal attribute (location = 1)
    const void* normalOffsetPtr = reinterpret_cast<const void*>(offsetof(VertexPN, mNormal));
    glVertexAttribPointer(1, // Attribute index 1
                          3, // Number of components (nx, ny, nz)
                          GL_FLOAT, // Type
                          GL_FALSE, // Normalized?
                          stride,   // Stride
                          normalOffsetPtr); // Offset of mNormal within VertexPN
    glEnableVertexAttribArray(1);   // Enable attribute 1

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}