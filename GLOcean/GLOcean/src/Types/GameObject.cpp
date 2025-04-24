#include "GameObject.h"
#include "../AssetManagement/AssetManager.h"

void GameObject::SetPosition(glm::vec3 position) {
    m_transform.position = position;
}

void GameObject::SetRotationY(float rotation) {
    m_transform.rotation.y = rotation;
}

void GameObject::SetModel(const std::string& name) {
    m_model = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName(name.c_str()));
    if (m_model) {
        m_meshMaterialIndices.resize(m_model->GetMeshCount());
        m_meshBlendingModes.resize(m_model->GetMeshCount());
        std::fill(m_meshMaterialIndices.begin(), m_meshMaterialIndices.end(), -1);
    }
    else {
        std::cout << "Failed to set model '" << name << "', it does not exist.\n";
    }
    for (BlendingMode& blendingMode : m_meshBlendingModes) {
        blendingMode = BlendingMode::NONE;
    }
}

void GameObject::SetMeshMaterialByMeshName(std::string meshName, const char* materialName) {
    int materialIndex = AssetManager::GetMaterialIndex(materialName);
    if (m_model && materialIndex != -1) {
        for (int i = 0; i < m_model->GetMeshCount(); i++) {
            if (AssetManager::GetMeshByIndex(m_model->GetMeshIndices()[i])->GetName() == meshName) {
                m_meshMaterialIndices[i] = materialIndex;
                return;
            }
        }
    }
    if (!m_model) {
        std::cout << "Tried to call SetMeshMaterialByMeshName() but this GameObject has a nullptr m_model\n";
    }
    else if (materialIndex == -1) {
        //std::cout << "Tried to call SetMeshMaterialByMeshName() but the material index was -1\n";
    }
    else {
        std::cout << "Tried to call SetMeshMaterialByMeshName() but the meshName '" << meshName << "' not found\n";
    }
}

void GameObject::SetMeshBlendingMode(const char* meshName, BlendingMode blendingMode) {
    // Range checks
    if (m_meshBlendingModes.empty()) {
        std::cout << "GameObject::SetMeshBlendingMode() failed: m_meshBlendingModes was empty!\n";
        return;
    }
    if (m_model) {
        bool found = false;
        for (int i = 0; i < m_model->GetMeshIndices().size(); i++) {
            OpenGLDetachedMesh* mesh = AssetManager::GetMeshByIndex(m_model->GetMeshIndices()[i]);
            if (mesh && mesh->GetName() == meshName) {
                m_meshBlendingModes[i] = blendingMode;
                found = true;
            }
        }
        if (!found) {
            std::cout << "GameObject::SetMeshBlendingMode() failed: " << meshName << " was not found!\n";
        }
    }
}

void GameObject::SetMeshBlendingModes(BlendingMode blendingMode) {
    if (m_model) {
        for (int i = 0; i < m_model->GetMeshIndices().size(); i++) {
            OpenGLDetachedMesh* mesh = AssetManager::GetMeshByIndex(m_model->GetMeshIndices()[i]);
            if (mesh) {
                m_meshBlendingModes[i] = blendingMode;
            }
        }
    }
}

void GameObject::SetName(const std::string& name) {
    m_name = name;
}

void GameObject::PrintMeshNames() {
    if (m_model) {
        std::cout << m_model->GetName() << "\n";
        for (uint32_t meshIndex : m_model->GetMeshIndices()) {
            OpenGLDetachedMesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            if (mesh) {
                std::cout << "-" << meshIndex << ": " << mesh->GetName() << "\n";
            }
        }
    }
}

void GameObject::UpdateRenderItems() {
    m_renderItems.clear();
    m_renderItemsBlended.clear();
    m_renderItemsAlphaDiscarded.clear();
    m_renderItemsHairTopLayer.clear();
    m_renderItemsHairBottomLayer.clear();
    if (m_model) {
        for (int i = 0; i < m_model->GetMeshCount(); i++) {
            OpenGLDetachedMesh* mesh = AssetManager::GetMeshByIndex(m_model->GetMeshIndices()[i]);
            if (mesh) {
                RenderItem renderItem;
                renderItem.modelMatrix = m_transform.to_mat4();
                renderItem.meshIndex = m_model->GetMeshIndices()[i];
                Material* material = AssetManager::GetMaterialByIndex(m_meshMaterialIndices[i]);
                if (material) {
                    renderItem.baseColorTextureIndex = material->m_basecolor;
                    renderItem.normalTextureIndex = material->m_normal;
                    renderItem.rmaTextureIndex = material->m_rma;
                }
                BlendingMode blendingMode = m_meshBlendingModes[i];
                switch (blendingMode) {
                case BlendingMode::NONE: m_renderItems.push_back(renderItem); break;
                case BlendingMode::BLENDED: m_renderItemsBlended.push_back(renderItem); break;
                case BlendingMode::ALPHA_DISCARDED: m_renderItemsAlphaDiscarded.push_back(renderItem); break;
                case BlendingMode::HAIR_TOP_LAYER: m_renderItemsHairTopLayer.push_back(renderItem); break;
                case BlendingMode::HAIR_UNDER_LAYER: m_renderItemsHairBottomLayer.push_back(renderItem); break;
                default: break;
                }
            }
        }
    }
}

std::vector<RenderItem>& GameObject::GetRenderItems() {
    return m_renderItems;
}

std::vector<RenderItem>& GameObject::GetRenderItemsBlended() {
    return m_renderItemsBlended;
}

std::vector<RenderItem>& GameObject::GetRenderItemsAlphaDiscarded() {
    return m_renderItemsAlphaDiscarded;
}

std::vector<RenderItem>& GameObject::GetRenderItemsHairTopLayer() {
    return m_renderItemsHairTopLayer;
}

std::vector<RenderItem>& GameObject::GetRenderItemsHairBottomLayer() {
    return m_renderItemsHairBottomLayer;
}
