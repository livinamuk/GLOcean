#pragma once
#include <vector>
#include "../AssetManagement/AssetManager.h"
#include "../Input/Input.h"
#include "../Types/GameObject.h"
#include "../Util.hpp"
#include "HellTypes.h"

namespace Scene {

    inline std::vector<GameObject> g_gameObjects;
    inline std::vector<RenderItem> g_renderItems;
    inline std::vector<RenderItem> g_renderItemsBlended;
    inline std::vector<RenderItem> g_renderItemsAlphaDiscarded;
    inline std::vector<RenderItem> g_renderItemsHairTopLayer;
    inline std::vector<RenderItem> g_renderItemsHairBottomLayer;

    inline void Init() {
        // nothing as of yet
    }

    inline void Update(float deltaTime) {
        // Clear global render item vectors
        g_renderItems.clear();
        g_renderItemsBlended.clear();
        g_renderItemsAlphaDiscarded.clear();
        g_renderItemsHairTopLayer.clear();
        g_renderItemsHairBottomLayer.clear();

        // Update each GameObject and collect render items
        for (GameObject& gameObject : g_gameObjects) {
            gameObject.UpdateRenderItems();

            if (gameObject.m_name == "Mermaid") {
                float amt = 0.5f;
                if (Input::KeyDown(HELL_KEY_LEFT)) {
                    gameObject.m_transform.rotation.y += deltaTime * amt;
                        std::cout << gameObject.m_transform.rotation.y << "\n";
                }
                if (Input::KeyDown(HELL_KEY_RIGHT)) {
                    gameObject.m_transform.rotation.y -= deltaTime * amt;
                        std::cout << gameObject.m_transform.rotation.y << "\n";
                }
            }

            // Merge render items into global vectors
            g_renderItems.insert(g_renderItems.end(), gameObject.GetRenderItems().begin(), gameObject.GetRenderItems().end());
            g_renderItemsBlended.insert(g_renderItemsBlended.end(), gameObject.GetRenderItemsBlended().begin(), gameObject.GetRenderItemsBlended().end());
            g_renderItemsAlphaDiscarded.insert(g_renderItemsAlphaDiscarded.end(), gameObject.GetRenderItemsAlphaDiscarded().begin(), gameObject.GetRenderItemsAlphaDiscarded().end());
            g_renderItemsHairTopLayer.insert(g_renderItemsHairTopLayer.end(), gameObject.GetRenderItemsHairTopLayer().begin(), gameObject.GetRenderItemsHairTopLayer().end());
            g_renderItemsHairBottomLayer.insert(g_renderItemsHairBottomLayer.end(), gameObject.GetRenderItemsHairBottomLayer().begin(), gameObject.GetRenderItemsHairBottomLayer().end());
        }
    }


    inline void CreateGameObject() {
        g_gameObjects.emplace_back();
    }

    inline GameObject* GetGameObjectByName(const std::string& name) {
        for (GameObject& gameObject : g_gameObjects) {
            if (gameObject.m_name == name) {
                return &gameObject;
            }
        }
        return nullptr;
    }

    inline void SetMaterials() {
        GameObject* room = GetGameObjectByName("Room");
        if (room) {
            room->SetMeshMaterialByMeshName("PlatformSide", "BathroomFloor");
            room->SetMeshMaterialByMeshName("PlatformTop", "BathroomFloor");
            room->SetMeshMaterialByMeshName("Floor", "BathroomFloor");
            room->SetMeshMaterialByMeshName("Ceiling", "Ceiling2");
            room->SetMeshMaterialByMeshName("WallZPos", "BathroomWall");
            room->SetMeshMaterialByMeshName("WallZNeg", "BathroomWall");
            room->SetMeshMaterialByMeshName("WallXPos", "BathroomWall");
            room->SetMeshMaterialByMeshName("WallXNeg", "BathroomWall");
        }

        GameObject* mermaid = GetGameObjectByName("Mermaid");
        if (mermaid) {
            mermaid->SetMeshMaterialByMeshName("Rock", "Rock");
            mermaid->SetMeshMaterialByMeshName("BoobTube", "BoobTube");
            mermaid->SetMeshMaterialByMeshName("Face", "MermaidFace");
            mermaid->SetMeshMaterialByMeshName("Body", "MermaidBody");
            mermaid->SetMeshMaterialByMeshName("Arms", "MermaidArms");
            mermaid->SetMeshMaterialByMeshName("HairInner", "MermaidHair");
            mermaid->SetMeshMaterialByMeshName("HairOutta", "MermaidHair");
            mermaid->SetMeshMaterialByMeshName("HairScalp", "MermaidScalp");
            mermaid->SetMeshMaterialByMeshName("EyeLeft", "MermaidEye");
            mermaid->SetMeshMaterialByMeshName("EyeRight", "MermaidEye");
            mermaid->SetMeshMaterialByMeshName("Tail", "MermaidTail");
            mermaid->SetMeshMaterialByMeshName("TailFin", "MermaidTail");
            mermaid->SetMeshMaterialByMeshName("EyelashUpper_HP", "MermaidLashes");
            mermaid->SetMeshMaterialByMeshName("EyelashLower_HP", "MermaidLashes");
            mermaid->SetMeshMaterialByMeshName("Nails", "Nails");
        }
    }

    inline void CreateGameObjects() {
        
        CreateGameObject();
        GameObject* mermaid = &g_gameObjects[g_gameObjects.size() - 1];
        mermaid->SetPosition(glm::vec3(18.0, -1.0f, 10.0f));
        mermaid->SetPosition(glm::vec3(0.0, -1.0f, 0.0f));
        mermaid->SetRotationY(3.14f * 1.7f);
        mermaid->SetModel("Mermaid");
        mermaid->SetMeshMaterialByMeshName("Rock", "Rock");
        mermaid->SetMeshMaterialByMeshName("BoobTube", "BoobTube");
        mermaid->SetMeshMaterialByMeshName("Face", "MermaidFace");
        mermaid->SetMeshMaterialByMeshName("Body", "MermaidBody");
        mermaid->SetMeshMaterialByMeshName("Arms", "MermaidArms");
        mermaid->SetMeshMaterialByMeshName("HairInner", "MermaidHair");
        mermaid->SetMeshMaterialByMeshName("HairOutta", "MermaidHair");
        mermaid->SetMeshMaterialByMeshName("HairScalp", "MermaidScalp");
        mermaid->SetMeshMaterialByMeshName("EyeLeft", "MermaidEye");
        mermaid->SetMeshMaterialByMeshName("EyeRight", "MermaidEye");
        mermaid->SetMeshMaterialByMeshName("Tail", "MermaidTail");
        mermaid->SetMeshMaterialByMeshName("TailFin", "MermaidTail");
        mermaid->SetMeshMaterialByMeshName("EyelashUpper_HP", "MermaidLashes");
        mermaid->SetMeshMaterialByMeshName("EyelashLower_HP", "MermaidLashes");
        mermaid->SetMeshMaterialByMeshName("Nails", "Nails");
        mermaid->SetMeshBlendingMode("EyelashUpper_HP", BlendingMode::BLENDED);
        mermaid->SetMeshBlendingMode("EyelashLower_HP", BlendingMode::BLENDED);
        mermaid->SetMeshBlendingMode("HairScalp", BlendingMode::BLENDED);
        mermaid->SetMeshBlendingMode("HairOutta", BlendingMode::HAIR_TOP_LAYER);
        mermaid->SetMeshBlendingMode("HairInner", BlendingMode::HAIR_UNDER_LAYER);
        mermaid->SetName("Mermaid");
    }

    inline std::vector<RenderItem>& GetRenderItems() { return g_renderItems; }
    inline std::vector<RenderItem>& GetRenderItemsBlended() { return g_renderItemsBlended; }
    inline std::vector<RenderItem>& GetRenderItemsAlphaDiscarded() { return g_renderItemsAlphaDiscarded; }
    inline std::vector<RenderItem>& GetRenderItemsHairTopLayer() { return g_renderItemsHairTopLayer; }
    inline std::vector<RenderItem>& GetRenderItemsHairBottomLayer() { return g_renderItemsHairBottomLayer; }
}