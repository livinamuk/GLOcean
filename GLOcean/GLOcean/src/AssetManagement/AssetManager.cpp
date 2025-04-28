#pragma once
#include "AssetManager.h"
#include <thread>
#include "../AssetManagement/BakeQueue.h"
#include "../API/OpenGL/GL_backend.h"
#include "../API/OpenGL/GL_util.hpp"
#include "../File/AssimpImporter.h"
#include "../Tools/ImageTools.h"
#include "../Util.hpp"

namespace AssetManager {

    std::vector<Texture> g_textures;
    std::vector<Material> g_materials;
    std::vector<Model> g_models;
    std::vector<OpenGLDetachedMesh> g_meshes;
    std::unordered_map<std::string, int> g_textureIndexMap;
    std::unordered_map<std::string, int> g_materialIndexMap;
    std::unordered_map<std::string, int> g_modelIndexMap;

    void LoadMinimum();
    void LoadModelsAsync();
    void LoadTexturesAsync();
    void LoadTexture(Texture* texture);
    void BuildMaterials();
    void LoadPendingTexturesAsync();
    void CompressMissingDDSTexutres();
    bool FileInfoIsAlbedoTexture(const FileInfo& fileInfo);
    std::string GetMaterialNameFromFileInfo(const FileInfo& fileInfo);

    void Init() {
        CompressMissingDDSTexutres();
        LoadMinimum();
        LoadModelsAsync();
        LoadTexturesAsync();
        BuildMaterials();
    }

    void Update() {
        for (Texture& texture : g_textures) {
            texture.CheckForBakeCompletion();
        }

        static bool baked = false;
        if (baked) {
            return;
        }

        for (Texture& texture : g_textures) {
            if (!texture.BakeComplete()) {
                return;
            }
        }
        std::cout << "done\n";
        baked = true;
        OpenGLBackend::CleanUpBakingPBOs();
    }

    void AddItemToLoadLog(std::string text) {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << text << "\n";
    }

    /*
    █▄ ▄█ █▀█ █▀▄ █▀▀ █
    █ █ █ █ █ █ █ █▀▀ █
    ▀   ▀ ▀▀▀ ▀▀  ▀▀▀ ▀▀▀ */

    void LoadModelsAsync() {
        // Scan for new obj and fbx and export custom model format
        for (FileInfo& fileInfo : Util::IterateDirectory("res/models_raw", { "obj", "fbx" })) {
            std::string assetPath = "res/models/" + fileInfo.name + ".model";

            // If the file exists but timestamps don't match, re-export
            if (Util::FileExists(assetPath)) {
                uint64_t lastModified = File::GetLastModifiedTime(fileInfo.path);
                ModelHeader modelHeader = File::ReadModelHeader(assetPath);
                if (modelHeader.timestamp != lastModified) {
                    File::DeleteFile(assetPath);
                    ModelData modelData = AssimpImporter::ImportFbx(fileInfo.path);
                    File::ExportModel(modelData);
                }
            }
            // File doesn't even exist yet, so export it
            else {
                ModelData modelData = AssimpImporter::ImportFbx(fileInfo.path);
                File::ExportModel(modelData);
            }
        }
        // Find and import all .model files
        for (FileInfo& fileInfo : Util::IterateDirectory("res/models")) {
            Model& model = g_models.emplace_back();
            ModelData modelData = File::ImportModel("res/models/" + fileInfo.GetFileNameWithExtension());
            LoadModelFromData(model, modelData);
        }

        // Build index map
        for (int i = 0; i < g_models.size(); i++) {
            g_modelIndexMap[g_models[i].GetName()] = i;
        }
    }

    /*
    █▄ ▄█ █▀▀ █▀▀ █ █
    █ █ █ █▀▀ ▀▀█ █▀█
    ▀   ▀ ▀▀▀ ▀▀▀ ▀ ▀ */

    int CreateMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, glm::vec3 aabbMin, glm::vec3 aabbMax) {
        OpenGLDetachedMesh& mesh = g_meshes.emplace_back();
        mesh.SetName(name);
        mesh.UpdateVertexBuffer(vertices, indices);
        mesh.aabbMin = aabbMin;
        mesh.aabbMax = aabbMax;
        return g_meshes.size() - 1;
    }

    int CreateMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());
        for (int i = 0; i < indices.size(); i += 3) {
            Vertex* vert0 = &vertices[indices[i]];
            Vertex* vert1 = &vertices[indices[i + 1]];
            Vertex* vert2 = &vertices[indices[i + 2]];
            glm::vec3 deltaPos1 = vert1->position - vert0->position;
            glm::vec3 deltaPos2 = vert2->position - vert0->position;
            glm::vec2 deltaUV1 = vert1->uv - vert0->uv;
            glm::vec2 deltaUV2 = vert2->uv - vert0->uv;
            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
            vert0->tangent = tangent;
            vert1->tangent = tangent;
            vert2->tangent = tangent;
            aabbMin = Util::Vec3Min(vert0->position, aabbMin);
            aabbMax = Util::Vec3Max(vert0->position, aabbMax);
            aabbMin = Util::Vec3Min(vert1->position, aabbMin);
            aabbMax = Util::Vec3Max(vert1->position, aabbMax);
            aabbMin = Util::Vec3Min(vert2->position, aabbMin);
            aabbMax = Util::Vec3Max(vert2->position, aabbMax);
        }
        OpenGLDetachedMesh& mesh = g_meshes.emplace_back();
        mesh.SetName(name);
        mesh.UpdateVertexBuffer(vertices, indices);
        mesh.aabbMin = aabbMin;
        mesh.aabbMax = aabbMax;
        return g_meshes.size() - 1;
    }

    int GetMeshIndexByName(const std::string& name) {
        for (int i = 0; i < g_meshes.size(); i++) {
            if (g_meshes[i].GetName() == name)
                return i;
        }
        std::cout << "AssetManager::GetMeshIndexByName() failed because '" << name << "' does not exist\n";
        return -1;
    }

    OpenGLDetachedMesh* GetDetachedMeshByName(const std::string& name) {
        for (int i = 0; i < g_meshes.size(); i++) {
            if (g_meshes[i].GetName() == name)
                return &g_meshes[i];
        }
        std::cout << "AssetManager::GetDetachedMeshByName() failed because '" << name << "' does not exist\n";
        return nullptr;
    }

    OpenGLDetachedMesh* GetMeshByIndex(int index) {
        if (index >= 0 && index < g_meshes.size()) {
            return &g_meshes[index];
        }
        else {
            std::cout << "AssetManager::GetMeshByIndex() failed because index '" << index << "' is out of range. Size is " << g_meshes.size() << "!\n";
            return nullptr;
        }
    }

    /*
     ▀█▀ █▀▀ █ █ ▀█▀ █ █ █▀▄ █▀▀
      █  █▀▀ ▄▀▄  █  █ █ █▀▄ █▀▀
      ▀  ▀▀▀ ▀ ▀  ▀  ▀▀▀ ▀ ▀ ▀▀▀ */

    void LoadMinimum() {
        // Find files
        for (FileInfo& fileInfo : Util::IterateDirectory("res/fonts", { "png" })) {
            Texture& texture = g_textures.emplace_back();
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
            texture.SetMinFilter(TextureFilter::NEAREST);
            texture.SetMagFilter(TextureFilter::NEAREST);
        }
        for (FileInfo& fileInfo : Util::IterateDirectory("res/textures/load_at_init/uncompressed", { "png", "jpg", })) {
            Texture& texture = g_textures.emplace_back();
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
            texture.SetMinFilter(TextureFilter::LINEAR_MIPMAP);
            texture.SetMagFilter(TextureFilter::LINEAR);
            texture.RequestMipmaps();
        }
        for (FileInfo& fileInfo : Util::IterateDirectory("res/textures/load_at_init/uncompressed_no_mipmaps", { "png", "jpg", })) {
            Texture& texture = g_textures.emplace_back();
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
            texture.SetMinFilter(TextureFilter::NEAREST);
            texture.SetMagFilter(TextureFilter::NEAREST);
        }
        // Async load them all
        LoadPendingTexturesAsync();

        // Immediate bake them
        BakeQueue::ImmediateBakeAllTextures();

        // Build index maps
        for (int i = 0; i < g_textures.size(); i++) {
            g_textureIndexMap[g_textures[i].GetFileInfo().name] = i;
        }
    }

    void LoadTexturesAsync() {
        // Find file paths
        //for (FileInfo& fileInfo : Util::IterateDirectory("res/textures/uncompressed", { "png", "jpg", "tga" })) {
        //    Texture& texture = g_textures.emplace_back();
        //    texture.SetFileInfo(fileInfo);
        //    texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
        //    texture.SetTextureWrapMode(TextureWrapMode::REPEAT);
        //    texture.SetMinFilter(TextureFilter::LINEAR_MIPMAP);
        //    texture.SetMagFilter(TextureFilter::LINEAR);
        //    texture.RequestMipmaps();
        //}
        for (FileInfo& fileInfo : Util::IterateDirectory("res/textures/compressed", { "dds" })) {
            Texture& texture = g_textures.emplace_back();
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::COMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::REPEAT);
            texture.SetMinFilter(TextureFilter::LINEAR_MIPMAP);
            texture.SetMagFilter(TextureFilter::LINEAR);
            texture.RequestMipmaps();
        }
        for (FileInfo& fileInfo : Util::IterateDirectory("res/textures/uncompressed_no_mipmaps", { "png", "jpg", })) {
            Texture& texture = g_textures.emplace_back();
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
            texture.SetMinFilter(TextureFilter::NEAREST);
            texture.SetMagFilter(TextureFilter::NEAREST);
        }
        // Async load them all
        LoadPendingTexturesAsync();

        // Build index maps
        for (int i = 0; i < g_textures.size(); i++) {
            g_textureIndexMap[g_textures[i].GetFileInfo().name] = i;
        }
    }

    void CompressMissingDDSTexutres() {
        for (FileInfo& fileInfo : Util::IterateDirectory("res/textures/compress_me", { "png", "jpg", "tga" })) {
            std::string inputPath = fileInfo.path;
            std::string outputPath = "res/textures/compressed/" + fileInfo.name + ".dds";
            if (!Util::FileExists(outputPath)) {
                ImageTools::CreateAndExportDDS(inputPath, outputPath, true);
                std::cout << "Exported " << outputPath << "\n";
            }
        }
    }

    void LoadPendingTexturesAsync() {
        std::vector<std::future<void>> futures;
        for (Texture& texture : g_textures) {
            if (texture.GetLoadingState() == LoadingState::AWAITING_LOADING_FROM_DISK) {
                texture.SetLoadingState(LoadingState::LOADING_FROM_DISK);
                futures.emplace_back(std::async(std::launch::async, LoadTexture, &texture));
            }
        }
        for (auto& future : futures) {
            future.get();
        }
        // Allocate gpu memory
        for (Texture& texture : g_textures) {
            OpenGLBackend::AllocateTextureMemory(texture);
        }
    }

    void LoadTexture(Texture* texture) {
        if (texture) {
            texture->Load();
            BakeQueue::QueueTextureForBaking(texture);
        }
    }

    Texture* GetTextureByName(const std::string& name) {
        for (Texture& texture : g_textures) {
            if (texture.GetFileInfo().name == name)
                return &texture;
        }
        std::cout << "AssetManager::GetTextureByName() failed because '" << name << "' does not exist\n";
        return nullptr;
    }

    int GetTextureIndexByName(const std::string& name, bool ignoreWarning) {
        for (int i = 0; i < g_textures.size(); i++) {
            if (g_textures[i].GetFileInfo().name == name)
                return i;
        }
        if (!ignoreWarning) {
            std::cout << "AssetManager::GetTextureIndexByName() failed because '" << name << "' does not exist\n";
        }
        return -1;
    }

    Texture* GetTextureByIndex(int index) {
        if (index != -1) {
            return &g_textures[index];
        }
        std::cout << "AssetManager::GetTextureByIndex() failed because index was -1\n";
        return nullptr;
    }

    int GetTextureCount() {
        return g_textures.size();
    }

    /*
    █▄ ▄█ █▀█ ▀█▀ █▀▀ █▀▄ ▀█▀ █▀█ █
    █ █ █ █▀█  █  █▀▀ █▀▄  █  █▀█ █
    ▀   ▀ ▀ ▀  ▀  ▀▀▀ ▀ ▀ ▀▀▀ ▀ ▀ ▀▀▀ */

    bool FileInfoIsAlbedoTexture(const FileInfo& fileInfo) {
        if (fileInfo.name.size() >= 4 && fileInfo.name.substr(fileInfo.name.size() - 4) == "_ALB") {
            return true;
        }
        return false;
    }

    std::string GetMaterialNameFromFileInfo(const FileInfo& fileInfo) {
        const std::string suffix = "_ALB";
        if (fileInfo.name.size() > suffix.size() && fileInfo.name.substr(fileInfo.name.size() - suffix.size()) == suffix) {
            return fileInfo.name.substr(0, fileInfo.name.size() - suffix.size());
        }
        return "";
    }

    void BuildMaterials() {
        g_materials.clear();
        for (Texture& texture : g_textures) {
            if (FileInfoIsAlbedoTexture(texture.GetFileInfo())) {
                Material& material = g_materials.emplace_back(Material());
                material.m_name = GetMaterialNameFromFileInfo(texture.GetFileInfo());
                int basecolorIndex = GetTextureIndexByName(material.m_name + "_ALB", true);
                int normalIndex = GetTextureIndexByName(material.m_name + "_NRM", true);
                int rmaIndex = GetTextureIndexByName(material.m_name + "_RMA", true);
                material.m_basecolor = basecolorIndex;
                material.m_normal = (normalIndex != -1) ? normalIndex : GetTextureIndexByName("DefaultNRM");
                material.m_rma = (rmaIndex != -1) ? rmaIndex : GetTextureIndexByName("DefaultRMA");
            }
        }
        // Build index maps
        for (int i = 0; i < g_materials.size(); i++) {
            g_materialIndexMap[g_materials[i].m_name] = i;
        }
    }

    

    Model* GetModelByIndex(int index) {
        if (index != -1) {
            return &g_models[index];
        }
        std::cout << "AssetManager::GetModelByIndex() failed because index was -1\n";
        return nullptr;
    }

    Model* CreateModel(const std::string& name) {
        g_models.emplace_back();
        Model* model = &g_models[g_models.size() - 1];
        model->SetName(name);
        return model;
    }

    void LoadModelFromData(Model& model, ModelData& modelData) {
        model.SetName(modelData.name);
        model.SetAABB(modelData.aabbMin, modelData.aabbMax);
        for (MeshData& meshData : modelData.meshes) {
            int meshIndex = CreateMesh(meshData.name, meshData.vertices, meshData.indices, meshData.aabbMin, meshData.aabbMax);
            model.AddMeshIndex(meshIndex);
        }
    }

    int GetModelIndexByName(const std::string& name) {
        auto it = g_modelIndexMap.find(name);
        if (it != g_modelIndexMap.end()) {
            return it->second;
        }
        std::cout << "AssetManager::GetModelIndexByName() failed because name '" << name << "' was not found in _models!\n";
        return -1;
    }

    Material* GetDefaultMaterial() {
        int index = GetMaterialIndex("CheckerBoard");
        return GetMaterialByIndex(index);
    }

    Material* GetMaterialByIndex(int index) {
        if (index >= 0 && index < g_materials.size()) {
            Material* material = &g_materials[index];
            Texture* baseColor = AssetManager::GetTextureByIndex(material->m_basecolor);
            Texture* normal = AssetManager::GetTextureByIndex(material->m_normal);
            Texture* rma = AssetManager::GetTextureByIndex(material->m_rma);
            if (baseColor && baseColor->BakeComplete() && 
                normal && normal->BakeComplete() && 
                rma && rma->BakeComplete()) {
                return &g_materials[index]; 
            }
            else {
                return GetDefaultMaterial();
            }
        }
        else {
            //std::cout << "AssetManager::GetMaterialByIndex() failed because index '" << index << "' is out of range. Size is " << g_materials.size() << "!\n";
            return GetDefaultMaterial();
        }
    }

    int GetMaterialIndex(const std::string& name) {
        auto it = g_materialIndexMap.find(name);
        if (it != g_materialIndexMap.end()) {
            return it->second;
        }
        else {
            std::cout << "AssetManager::GetMaterialIndex() failed because '" << name << "' does not exist\n";
            return -1;
        }
    }

    std::string& GetMaterialNameByIndex(int index) {
        return g_materials[index].m_name;
    }
}




    



    

