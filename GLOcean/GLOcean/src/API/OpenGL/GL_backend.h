#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include "Types/GL_pbo.hpp"
#include "Types/GL_texture.h"
#include "../Types/Texture.h"

enum class WindowedMode { WINDOWED, FULLSCREEN };

namespace OpenGLBackend {
    void Init(std::string title);
    void ToggleFullscreen();
    void SetWindowedMode(const WindowedMode& windowedMode);
    void ToggleFullscreen();
    void ForceCloseWindow();
    void CleanUpBakingPBOs();
    double GetMouseX();
    double GetMouseY();
    int GetWindowWidth();
    int GetWindowHeight();
    void SwapBuffersPollEvents();
    bool WindowIsOpen();
    GLFWwindow* GetWindowPtr();

    // Textures
    void UpdateTextureBaking();
    void AllocateTextureMemory(Texture& texture);
    void ImmediateBake(QueuedTextureBake& queuedTextureBake);
    void AsyncBakeQueuedTextureBake(QueuedTextureBake& queuedTextureBake);
}