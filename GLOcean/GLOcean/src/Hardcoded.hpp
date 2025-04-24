#pragma once
#include "AssetManagement/AssetManager.h"

namespace Hardcoded {

    inline float roomWidth = 7;
    inline float roomDepth = 10;
    inline float roomY = -1.6f;
    inline float roomHeight = 2.6f;
    inline float platfromHeight = 1.1f;
    inline float platfromDepth = 2.5f;
    inline float waterHeight = 0.95f;
    inline glm::vec3 bottomFrontLeft = glm::vec3(0, roomY, 0);
    inline glm::vec3 bottomFrontRight = glm::vec3(roomWidth, roomY, 0);
    inline glm::vec3 bottomBackLeft = glm::vec3(0, roomY, roomDepth);
    inline glm::vec3 bottomBackRight = glm::vec3(roomWidth, roomY, roomDepth);
    inline glm::vec3 topFrontLeft = bottomFrontLeft + glm::vec3(0, roomHeight, 0);
    inline glm::vec3 topFrontRight = bottomFrontRight + glm::vec3(0, roomHeight, 0);
    inline glm::vec3 topBackLeft = bottomBackLeft + glm::vec3(0, roomHeight, 0);
    inline glm::vec3 topBackRight = bottomBackRight + glm::vec3(0, roomHeight, 0);
    inline glm::vec3 platformBottomFrontLeft = glm::vec3(0, roomY, platfromDepth);
    inline glm::vec3 platformBottomFrontRight = glm::vec3(roomWidth, roomY, platfromDepth);
    inline glm::vec3 platformTopFrontLeft = platformBottomFrontLeft + glm::vec3(0, platfromHeight, 0);
    inline glm::vec3 platformTopFrontRight = platformBottomFrontRight + glm::vec3(0, platfromHeight, 0);
    inline glm::vec3 up = glm::vec3(0, 1, 0);
    inline glm::vec3 down = glm::vec3(0, -1, 0);
    inline glm::vec3 xPos = glm::vec3(-1, 0, 0);
    inline glm::vec3 xNeg = glm::vec3(1, 0, 0);
    inline glm::vec3 zPos = glm::vec3(0, 0, 1);
    inline glm::vec3 zNeg = glm::vec3(0, 0, -1);

    inline void LoadQuadModel() {
        Model* model = AssetManager::CreateModel("Quads");
        std::vector<Vertex> vertices;
        vertices.push_back(Vertex(glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)));
        vertices.push_back(Vertex(glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)));
        vertices.push_back(Vertex(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)));
        vertices.push_back(Vertex(glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)));
        std::vector<uint32_t> indices = { 2, 1, 0, 3, 2, 0 };
        model->AddMeshIndex(AssetManager::CreateMesh("FlipBookQuadCentered", vertices, indices));

        vertices.clear();
        vertices.push_back(Vertex(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)));
        vertices.push_back(Vertex(glm::vec3(-1.0f, 2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)));
        vertices.push_back(Vertex(glm::vec3(1.0f, 2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)));
        vertices.push_back(Vertex(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)));
        indices = { 2, 1, 0, 3, 2, 0 };
        model->AddMeshIndex(AssetManager::CreateMesh("FlipBookQuadBottomAligned", vertices, indices));
    }

    inline void LoadHardcodedWaterModel() {
        Model* model = AssetManager::CreateModel("Water");
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        vertices.push_back(Vertex(bottomFrontLeft + glm::vec3(0, waterHeight, 0), up, glm::vec2(0, 0)));
        vertices.push_back(Vertex(bottomFrontRight + glm::vec3(0, waterHeight, 0), up, glm::vec2(roomWidth, 0)));
        vertices.push_back(Vertex(bottomBackLeft + glm::vec3(0, waterHeight, 0), up, glm::vec2(0, roomDepth)));
        vertices.push_back(Vertex(bottomBackRight + glm::vec3(0, waterHeight, 0), up, glm::vec2(roomWidth, roomDepth)));
        vertices.push_back(Vertex(bottomFrontLeft + glm::vec3(0, waterHeight, 0), down, glm::vec2(0, 0)));
        vertices.push_back(Vertex(bottomFrontRight + glm::vec3(0, waterHeight, 0), down, glm::vec2(roomWidth, 0)));
        vertices.push_back(Vertex(bottomBackLeft + glm::vec3(0, waterHeight, 0), down, glm::vec2(0, roomDepth)));
        vertices.push_back(Vertex(bottomBackRight + glm::vec3(0, waterHeight, 0), down, glm::vec2(roomWidth, roomDepth)));
        indices = { 0, 2, 1, 2, 3, 1, 0 + 4, 1 + 4, 2 + 4, 2 + 4, 1 + 4, 3 + 4 };
        model->AddMeshIndex(AssetManager::CreateMesh("Water", vertices, indices));
    }

    inline void LoadHardcodedRoomModel() {
        Model* model = AssetManager::CreateModel("Room");
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        // Platform Side
        vertices = {};
        vertices.push_back(Vertex(platformBottomFrontLeft, zPos));
        vertices.push_back(Vertex(platformBottomFrontRight, zPos));
        vertices.push_back(Vertex(platformTopFrontLeft, zPos));
        vertices.push_back(Vertex(platformTopFrontRight, zPos));
        for (int i = 0; i < 4; i++) {
            vertices[i].uv = glm::vec2(vertices[i].position.x, vertices[i].position.y);
        }
        indices = { 0, 1, 2, 2, 1, 3 };
        model->AddMeshIndex(AssetManager::CreateMesh("PlatformSide", vertices, indices));
        // Platform Top
        vertices = {};
        vertices.push_back(Vertex(platformTopFrontLeft, up));
        vertices.push_back(Vertex(platformTopFrontRight, up));
        vertices.push_back(Vertex(platformTopFrontLeft * glm::vec3(1, 1, 0), up));
        vertices.push_back(Vertex(platformTopFrontRight * glm::vec3(1, 1, 0), up));
        for (int i = 0; i < 4; i++) {
            vertices[i].uv = glm::vec2(vertices[i].position.x, vertices[i].position.z);
        }
        indices = { 0, 1, 2, 2, 1, 3 };
        model->AddMeshIndex(AssetManager::CreateMesh("PlatformTop", vertices, indices));
        // Floor
        vertices = {};
        vertices.push_back(Vertex(bottomFrontLeft, up, glm::vec2(0, 0)));
        vertices.push_back(Vertex(bottomFrontRight, up, glm::vec2(roomWidth, 0)));
        vertices.push_back(Vertex(bottomBackLeft, up, glm::vec2(0, roomDepth)));
        vertices.push_back(Vertex(bottomBackRight, up, glm::vec2(roomWidth, roomDepth)));
        indices = { 0, 2, 1, 2, 3, 1 };
        model->AddMeshIndex(AssetManager::CreateMesh("Floor", vertices, indices));
        // Ceiling
        vertices = {};
        vertices.push_back(Vertex(topFrontLeft, down, glm::vec2(0, 0)));
        vertices.push_back(Vertex(topFrontRight, down, glm::vec2(roomWidth, 0)));
        vertices.push_back(Vertex(topBackLeft, down, glm::vec2(0, roomDepth)));
        vertices.push_back(Vertex(topBackRight, down, glm::vec2(roomWidth, roomDepth)));
        indices = { 0, 1, 2, 2, 1, 3 };
        model->AddMeshIndex(AssetManager::CreateMesh("Ceiling", vertices, indices));
        // Z Pos
        vertices = {};
        vertices.push_back(Vertex(bottomFrontLeft, zPos, glm::vec2(0, 0)));
        vertices.push_back(Vertex(bottomFrontRight, zPos, glm::vec2(roomWidth / roomHeight, 0)));
        vertices.push_back(Vertex(topFrontLeft, zPos, glm::vec2(0, -1)));
        vertices.push_back(Vertex(topFrontRight, zPos, glm::vec2(roomWidth / roomHeight, -1)));
        indices = { 0, 1, 2, 2, 1, 3 };
        model->AddMeshIndex(AssetManager::CreateMesh("WallZPos", vertices, indices));
        // Z Neg
        vertices = {};
        vertices.push_back(Vertex(bottomBackLeft, zNeg, glm::vec2(0, 0)));
        vertices.push_back(Vertex(bottomBackRight, zNeg, glm::vec2(roomWidth / roomHeight, 0)));
        vertices.push_back(Vertex(topBackLeft, zNeg, glm::vec2(0, -1)));
        vertices.push_back(Vertex(topBackRight, zNeg, glm::vec2(roomWidth / roomHeight, -1)));
        indices = { 0, 2, 1, 2, 3, 1 };
        model->AddMeshIndex(AssetManager::CreateMesh("WallZNeg", vertices, indices));
        // X Pos
        vertices = {};
        vertices.push_back(Vertex(bottomFrontRight, xPos, glm::vec2(0, 0)));
        vertices.push_back(Vertex(bottomBackRight, xPos, glm::vec2(roomDepth / roomHeight, 0)));
        vertices.push_back(Vertex(topFrontRight, xPos, glm::vec2(0, -1)));
        vertices.push_back(Vertex(topBackRight, xPos, glm::vec2(roomDepth / roomHeight, -1)));
        indices = { 0, 1, 2, 2, 1, 3 };
        model->AddMeshIndex(AssetManager::CreateMesh("WallXPos", vertices, indices));
        // X Neg
        vertices = {};
        vertices.push_back(Vertex(bottomFrontLeft, xNeg, glm::vec2(0, 0)));
        vertices.push_back(Vertex(bottomBackLeft, xNeg, glm::vec2(roomDepth / roomHeight, 0)));
        vertices.push_back(Vertex(topFrontLeft, xNeg, glm::vec2(0, -1)));
        vertices.push_back(Vertex(topBackLeft, xNeg, glm::vec2(roomDepth / roomHeight, -1)));
        indices = { 0, 2, 1, 2, 3, 1 };
        model->AddMeshIndex(AssetManager::CreateMesh("WallXNeg", vertices, indices));
    }
}