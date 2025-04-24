#pragma once
#include "Common.h"
#include <string>
#include <vector>

struct Model {

private:
    std::string m_name = "undefined";
    std::vector<uint32_t> m_meshIndices;
public:
    glm::vec3 m_aabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 m_aabbMax = glm::vec3(-std::numeric_limits<float>::max());
    bool m_awaitingLoadingFromDisk = true;
    bool m_loadedFromDisk = false;
    std::string m_fullPath = ""; 

public:

    Model() = default;

    Model(std::string fullPath) {
        m_fullPath = fullPath;
        m_name = fullPath.substr(fullPath.rfind("/") + 1);
        m_name = m_name.substr(0, m_name.length() - 4);
    }

    void AddMeshIndex(uint32_t index) {
        m_meshIndices.push_back(index);
    }

    size_t GetMeshCount() {
        return m_meshIndices.size();
    }

    std::vector<uint32_t>& GetMeshIndices() {
        return m_meshIndices;
    }

    void SetName(std::string modelName) {
        m_name = modelName;
    }

    void SetAABB(glm::vec3 aabbMin, glm::vec3 aabbMax) {
        m_aabbMin = aabbMin;
        m_aabbMax = aabbMax;
    }

    const std::string GetName() {
        return m_name;
    }
};