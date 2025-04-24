#pragma once
#include <string>
#include "../API/OpenGL/Types/GL_detachedMesh.hpp"
#include "HellTypes.h"
#include "../File/File.h"
#include "../Types/Model.hpp"
#include "../Types/Texture.h"

namespace AssetManager {
    void Init();
    void Update();

    // Textures
    int GetTextureIndexByName(const std::string& name, bool ignoreWarning = true);
    int GetTextureCount();
    Texture* GetTextureByName(const std::string& name);
    Texture* GetTextureByIndex(int index);

    // Models
    void LoadModelFromData(Model& model, ModelData& modelData);
    int GetModelIndexByName(const std::string& name);
    Model* CreateModel(const std::string& name);
    Model* GetModelByIndex(int index);

    // Mesh
    int CreateMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, glm::vec3 aabbMin, glm::vec3 aabbMax);
    int CreateMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    int GetMeshIndexByName(const std::string& name);
    int GetMeshIndexByName(const std::string& name);
    OpenGLDetachedMesh* GetDetachedMeshByName(const std::string& name);
    OpenGLDetachedMesh* GetMeshByIndex(int index);

    // Materials
    Material* GetDefaultMaterial();
    Material* GetMaterialByIndex(int index);
    int GetMaterialIndex(const std::string& name);
    std::string& GetMaterialNameByIndex(int index);
}