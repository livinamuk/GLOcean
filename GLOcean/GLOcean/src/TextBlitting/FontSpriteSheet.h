#pragma once
#include <string>
#include <vector>

struct FontSpriteSheet {
    std::string m_name;
    std::string m_characters;
    int m_textureWidth;
    int m_textureHeight;
    int m_lineHeight;
    struct CharData {
        int width;
        int height;
        int offsetX;
        int offsetY;
    };
    std::vector<CharData> m_charDataList;
};

namespace FontSpriteSheetPacker {
    void Export(const std::string& name, 
        const std::string& characters, 
        const std::string& textureSourcePath, 
        const std::string& outputPath
    );
    FontSpriteSheet Import(const std::string& filepath);
    void ExampleUsage();
}