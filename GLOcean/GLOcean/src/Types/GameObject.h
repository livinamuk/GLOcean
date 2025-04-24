#pragma once
#include "HellTypes.h"
#include "Model.hpp"

struct GameObject {
    std::string m_name;
    Model* m_model = nullptr;
    Transform m_transform;
    std::vector<BlendingMode> m_meshBlendingModes;
    std::vector<int> m_meshMaterialIndices;

    void SetName(const std::string& name);
    void SetPosition(glm::vec3 position);
    void SetRotationY(float rotation);
    void SetModel(const std::string& name);
    void SetMeshMaterialByMeshName(std::string meshName, const char* materialName);
    void SetMeshBlendingMode(const char* meshName, BlendingMode blendingMode);
    void SetMeshBlendingModes(BlendingMode blendingMode);
    void PrintMeshNames();
    void UpdateRenderItems();

    std::vector<RenderItem>& GetRenderItems();
    std::vector<RenderItem>& GetRenderItemsBlended();
    std::vector<RenderItem>& GetRenderItemsAlphaDiscarded();
    std::vector<RenderItem>& GetRenderItemsHairTopLayer();
    std::vector<RenderItem>& GetRenderItemsHairBottomLayer();

private:
    std::vector<RenderItem> m_renderItems;
    std::vector<RenderItem> m_renderItemsBlended;
    std::vector<RenderItem> m_renderItemsAlphaDiscarded;
    std::vector<RenderItem> m_renderItemsHairTopLayer;
    std::vector<RenderItem> m_renderItemsHairBottomLayer;
};