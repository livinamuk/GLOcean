#pragma once
#include <string>
#include "../API/OpenGL/Types/GL_fontMesh.hpp"

namespace TextBlitter {

    void Init();
    void BlitText(const std::string& text, 
        const std::string fontName, 
        int locationX, 
        int locationY, 
        int viewportWidth, 
        int viewportHeight,
        float scale
    );
    void Update();

    OpenGLFontMesh* GetGLFontMesh(const std::string& name);
}