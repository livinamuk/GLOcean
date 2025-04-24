#include "TextBlitter.h"
#include "FontSpriteSheet.h"
#include <unordered_map>
#include <iostream>

namespace TextBlitter {

    struct FontMeshData {
        std::vector<FontVertex> vertices;
        std::vector<uint32_t> indices;
    };

    std::vector<FontSpriteSheet> g_fontSpriteSheets;
    std::vector<FontMeshData> g_fontMeshDataMaps;
    std::vector<OpenGLFontMesh> g_glFontMeshs;
    std::unordered_map<std::string, uint32_t> g_fontIndices;

    void AddFont(const FontSpriteSheet& font);
    FontSpriteSheet* GetFontSpriteSheet(const std::string& name);
    FontMeshData* GetFontMeshData(const std::string& name);
    glm::vec3 ParseColorTag(const std::string& tag);

    void Init() {
        // Export standard font (no need to do every init but YOLO ¯\_(ツ)_/¯
        std::string name = "StandardFont";
        std::string characters = R"(!"#$%&'*+,-./0123456789:;<=>?_ABCDEFGHIJKLMNOPQRSTUVWXYZ\^_`abcdefghijklmnopqrstuvwxyz )";
        std::string textureSourcePath = "res/fonts/raw_images/standard_font/";
        std::string outputPath = "res/fonts/";
        FontSpriteSheetPacker::Export(name, characters, textureSourcePath, outputPath);

        // Import fonts
        FontSpriteSheet standardFont = FontSpriteSheetPacker::Import("res/fonts/StandardFont.json");
        AddFont(standardFont);
    }

    void Update() {
        for (FontSpriteSheet& spriteSheet : g_fontSpriteSheets) {
            FontMeshData* meshData = GetFontMeshData(spriteSheet.m_name);
            OpenGLFontMesh* mesh = GetGLFontMesh(spriteSheet.m_name);
            if (mesh && meshData) {
                mesh->UpdateVertexBuffer(meshData->vertices, meshData->indices);
                meshData->vertices.clear();
                meshData->indices.clear();
            }
        }
    }

    void BlitText(const std::string& text, const std::string fontName, int locationX, int locationY, int viewportWidth, int viewportHeight, float scale) {
        FontSpriteSheet* spriteSheet = GetFontSpriteSheet(fontName);
        FontMeshData* meshData = GetFontMeshData(fontName);

        if (spriteSheet && meshData) {
            float cursorX = static_cast<float>(locationX);
            float cursorY = static_cast<float>(viewportHeight - locationY); // Top left corner
            float invTextureWidth = 1.0f / static_cast<float>(spriteSheet->m_textureWidth);
            float invTextureHeight = 1.0f / static_cast<float>(spriteSheet->m_textureHeight);
            float halfPixelU = 0.5f * invTextureWidth;
            float halfPixelV = 0.5f * invTextureHeight;
            glm::vec3 color(1.0f); // Default color

            // Reserve space for vertices and indices
            size_t estimatedVertices = text.length() * 4;
            size_t estimatedIndices = text.length() * 6;
            meshData->vertices.reserve(meshData->vertices.size() + estimatedVertices);
            meshData->indices.reserve(meshData->indices.size() + estimatedIndices);

            for (size_t i = 0; i < text.length();) {

                // Handle color tags
                if (text.compare(i, 5, "[COL=") == 0) {
                    size_t end = text.find("]", i);
                    if (end != std::string::npos) {
                        color = ParseColorTag(text.substr(i, end - i + 1));
                        i = end; // Skip the tag
                        continue;
                    }
                }
                char character = text[i];

                // Handle spaces
                if (character == ' ') {
                    int spaceWidth = spriteSheet->m_charDataList[spriteSheet->m_characters.find(' ')].width;
                    cursorX += spaceWidth * scale;
                    i++;
                    continue;
                }
                // Handle newlines
                if (character == '\n') {
                    cursorX = static_cast<float>(locationX);
                    cursorY -= (spriteSheet->m_lineHeight + 1) * scale;
                    i++;
                    continue;
                }
                // Process regular characters
                int charIndex = spriteSheet->m_characters.find(character);
                if (charIndex != std::string::npos) {
                    const auto& charData = spriteSheet->m_charDataList[charIndex];

                    // Normalized uvs
                    float u0 = (charData.offsetX + halfPixelU) * invTextureWidth;
                    float v0 = (charData.offsetY + charData.height - halfPixelV) * invTextureHeight;
                    float u1 = (charData.offsetX + charData.width - halfPixelU) * invTextureWidth;
                    float v1 = (charData.offsetY + halfPixelV) * invTextureHeight;

                    // Normalized quad position
                    float x0 = (cursorX / viewportWidth) * 2.0f - 1.0f;
                    float y0 = (cursorY / viewportHeight) * 2.0f - 1.0f;
                    float x1 = ((cursorX + charData.width * scale) / viewportWidth) * 2.0f - 1.0f;
                    float y1 = ((cursorY - charData.height * scale) / viewportHeight) * 2.0f - 1.0f;

                    // Vertices
                    meshData->vertices.push_back({ {x0, y0}, {u0, v1}, color }); // Bottom left
                    meshData->vertices.push_back({ {x1, y0}, {u1, v1}, color }); // Bottom right
                    meshData->vertices.push_back({ {x1, y1}, {u1, v0}, color }); // Top right
                    meshData->vertices.push_back({ {x0, y1}, {u0, v0}, color }); // Top left

                    // Indices
                    uint32_t baseIndex = static_cast<uint32_t>(meshData->vertices.size()) - 4;
                    meshData->indices.push_back(baseIndex + 0);
                    meshData->indices.push_back(baseIndex + 1);
                    meshData->indices.push_back(baseIndex + 2);
                    meshData->indices.push_back(baseIndex + 0);
                    meshData->indices.push_back(baseIndex + 2);
                    meshData->indices.push_back(baseIndex + 3);

                    cursorX += charData.width * scale;
                }
                i++;
            }
        }
    }

    void AddFont(const FontSpriteSheet& spriteSheet) {
        if (g_fontIndices.find(spriteSheet.m_name) != g_fontIndices.end()) {
            std::cout << "Font already exists: " << spriteSheet.m_name << "\n";
            return;
        }
        uint32_t index = g_fontSpriteSheets.size();
        g_fontIndices[spriteSheet.m_name] = index;
        g_fontSpriteSheets.push_back(spriteSheet);
        g_fontMeshDataMaps.emplace_back();
        g_glFontMeshs.emplace_back();
    }

    FontSpriteSheet* GetFontSpriteSheet(const std::string& name) {
        auto it = g_fontIndices.find(name);
        return (it != g_fontIndices.end()) ? &g_fontSpriteSheets[it->second] : nullptr;
    }

    FontMeshData* GetFontMeshData(const std::string& name) {
        auto it = g_fontIndices.find(name);
        return (it != g_fontIndices.end()) ? &g_fontMeshDataMaps[it->second] : nullptr;
    }

    OpenGLFontMesh* GetGLFontMesh(const std::string& name) {
        auto it = g_fontIndices.find(name);
        return (it != g_fontIndices.end()) ? &g_glFontMeshs[it->second] : nullptr;
    }

    glm::vec3 ParseColorTag(const std::string& tag) {
        glm::vec3 color(1.0f);
        size_t start = tag.find("[COL=") + 5;
        if (start == std::string::npos) {
            return color;
        }
        size_t end = tag.find("]", start);
        if (end == std::string::npos) {
            return color;
        }
        const char* cStr = tag.c_str() + start;
        char* endPtr;
        color.r = std::strtof(cStr, &endPtr);
        if (*endPtr != ',') {
            return color;
        }
        color.g = std::strtof(endPtr + 1, &endPtr);
        if (*endPtr != ',') {
            return color;
        }
        color.b = std::strtof(endPtr + 1, &endPtr);
        return color;
    }
}