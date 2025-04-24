#pragma once

#include <string>
#include <memory>
#include <string>
#include <vector>
#include <future>
#include <thread>
#include <numeric>
#include <algorithm>
#include <numeric>
#include <filesystem>
#include <unordered_map>

#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Enums.h"
#include "HellTypes.h"

#define ORANGE  glm::vec3(1, 0.647f, 0)
#define BLACK   glm::vec3(0, 0, 0)
#define WHITE   glm::vec3(1, 1, 1)
#define RED     glm::vec3(1, 0, 0)
#define GREEN   glm::vec3(0, 1, 0)
#define BLUE    glm::vec3(0, 0, 1)
#define YELLOW  glm::vec3(1, 1, 0)
#define PURPLE  glm::vec3(1, 0, 1)
#define GREY    glm::vec3(0.25f)
#define LIGHT_BLUE    glm::vec3(0, 1, 1)
#define LIGHT_GREEN   glm::vec3(0.16f, 0.78f, 0.23f)
#define LIGHT_RED     glm::vec3(0.8f, 0.05f, 0.05f)

struct AABB {
    AABB() {}
    AABB(glm::vec3 min, glm::vec3 max) {
        boundsMin = min;
        boundsMax = max;
        CalculateCenter();
    }
    void Grow(AABB& b) {
        if (b.boundsMin.x != 1e30f && b.boundsMin.x != -1e30f) {
            Grow(b.boundsMin); Grow(b.boundsMax);
        }
        CalculateCenter();
    }
    void Grow(glm::vec3 p) {
        boundsMin = glm::vec3(std::min(boundsMin.x, p.x), std::min(boundsMin.y, p.y), std::min(boundsMin.z, p.z));
        boundsMax = glm::vec3(std::max(boundsMax.x, p.x), std::max(boundsMax.y, p.y), std::max(boundsMax.z, p.z));
        CalculateCenter();
    }
    float Area() {
        glm::vec3 e = boundsMax - boundsMin; // box extent
        return e.x * e.y + e.y * e.z + e.z * e.x;
    }
    const glm::vec3 GetCenter() {
        return center;
    }
    const glm::vec3 GetBoundsMin() {
        return boundsMin;
    }
    const glm::vec3 GetBoundsMax() {
        return boundsMax;
    }

public: // make private later
    glm::vec3 center = glm::vec3(0);
    glm::vec3 boundsMin = glm::vec3(1e30f);
    glm::vec3 boundsMax = glm::vec3(-1e30f);
    glm::vec3 padding = glm::vec3(0);

    void CalculateCenter() {
        center = { (boundsMin.x + boundsMax.x) / 2, (boundsMin.y + boundsMax.y) / 2, (boundsMin.z + boundsMax.z) / 2 };
    }

    bool ContainsPoint(glm::vec3 point) const {
        return (point.x >= boundsMin.x && point.x <= boundsMax.x) &&
            (point.y >= boundsMin.y && point.y <= boundsMax.y) &&
            (point.z >= boundsMin.z && point.z <= boundsMax.z);
    }
};