#pragma once
#include "Common.h"
#include "Types/GL_detachedMesh.hpp"

namespace OpenGLRenderer {

    void Init();
    void LoadShaders();
    void RenderFrame();

    // Debug
    void UpdateDebugLinesMesh();
    void UpdateDebugPointsMesh();
    void DrawPoint(glm::vec3 position, glm::vec3 color);
    void DrawLine(glm::vec3 begin, glm::vec3 end, glm::vec3 color);
    void DrawAABB(AABB& aabb, glm::vec3 color);
    void DrawAABB(AABB& aabb, glm::vec3 color, glm::mat4 worldTransform);
    inline std::vector<Vertex> g_debugLines;
    inline std::vector<Vertex> g_debugPoints;
    inline OpenGLDetachedMesh g_debugLinesMesh;
    inline OpenGLDetachedMesh g_debugPointsMesh;
}