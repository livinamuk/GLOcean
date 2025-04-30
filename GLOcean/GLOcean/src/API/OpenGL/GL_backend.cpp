#pragma once
#include "GL_backend.h"
#include "../AssetManagement/BakeQueue.h"
#include "../AssetManagement/AssetManager.h"
#include <string>
#include <iostream>
#include <vector>
#include "HellTypes.h"
#include "GL_util.hpp"

namespace OpenGLBackend {

    GLFWwindow* g_window;
    WindowedMode g_windowedMode;
    GLFWmonitor* g_monitor;
    const GLFWvidmode* g_mode;
    GLuint g_currentWindowWidth = 0;
    GLuint g_currentWindowHeight = 0;
    GLuint g_windowedWidth = 0;
    GLuint g_windowedHeight = 0;
    GLuint g_fullscreenWidth;
    GLuint g_fullscreenHeight;

    // PBO texture loading
    const size_t MAX_TEXTURE_WIDTH = 4096;
    const size_t MAX_TEXTURE_HEIGHT = 4096;
    const size_t MAX_CHANNEL_COUNT = 4;
    const size_t MAX_DATA_SIZE = MAX_TEXTURE_WIDTH * MAX_TEXTURE_HEIGHT * MAX_CHANNEL_COUNT;
    std::vector<PBO> g_textureBakingPBOs;

    GLFWwindow* GetWindowPtr() {
        return g_window;
    }

    void Init(std::string title) {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
        g_monitor = glfwGetPrimaryMonitor();
        g_mode = glfwGetVideoMode(g_monitor);
        glfwWindowHint(GLFW_RED_BITS, g_mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, g_mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, g_mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, g_mode->refreshRate);
        g_fullscreenWidth = g_mode->width;
        g_fullscreenHeight = g_mode->height;
        g_windowedWidth = g_fullscreenWidth * 0.75f;
        g_windowedHeight = g_fullscreenHeight * 0.75f;
        g_window = glfwCreateWindow(g_windowedWidth, g_windowedHeight, title.c_str(), NULL, NULL);
        if (g_window == NULL) {
            std::cout << "GLFW window failed creation\n";
            glfwTerminate();
            return;
        }
        glfwMakeContextCurrent(g_window);
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cout << "GLAD failed init\n";
            return;
        }
        GLint major, minor;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        const GLubyte* vendor = glGetString(GL_VENDOR);
        const GLubyte* renderer = glGetString(GL_RENDERER);
        std::cout << "\nGPU: " << renderer << "\n";
        std::cout << "GL version: " << major << "." << minor << "\n\n";

        glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        ToggleFullscreen();
        ToggleFullscreen();

        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        SwapBuffersPollEvents();

        for (int i = 0; i < 16; ++i) {
            PBO& pbo = g_textureBakingPBOs.emplace_back();
            pbo.Init(MAX_DATA_SIZE);
        }
        return;
    }

    void SetWindowedMode(const WindowedMode& windowedMode) {
        if (windowedMode == WindowedMode::WINDOWED) {
            g_currentWindowWidth = g_windowedWidth;
            g_currentWindowHeight = g_windowedHeight;
            glfwSetWindowMonitor(g_window, nullptr, 0, 0, g_windowedWidth, g_windowedHeight, g_mode->refreshRate);
            glfwSetWindowPos(g_window, 0, 0);
            //glfwSwapInterval(0);
        }
        else if (windowedMode == WindowedMode::FULLSCREEN) {
            g_currentWindowWidth = g_fullscreenWidth;
            g_currentWindowHeight = g_fullscreenHeight;
            glfwSetWindowMonitor(g_window, g_monitor, 0, 0, g_fullscreenWidth, g_fullscreenHeight, g_mode->refreshRate); 
            //glfwSwapInterval(1);
        }
        g_windowedMode = windowedMode;
    }

    void ToggleFullscreen() {
        if (g_windowedMode == WindowedMode::WINDOWED) {
            SetWindowedMode(WindowedMode::FULLSCREEN);
        }
        else {
            SetWindowedMode(WindowedMode::WINDOWED);
        }
    }

    void ForceCloseWindow() {
        glfwSetWindowShouldClose(g_window, true);
    }

    double GetMouseX() {
        double x, y;
        glfwGetCursorPos(g_window, &x, &y);
        return x;
    }

    double GetMouseY() {
        double x, y;
        glfwGetCursorPos(g_window, &x, &y);
        return y;
    }

    int GetWindowWidth() {
        int width, height;
        glfwGetWindowSize(g_window, &width, &height);
        return width;
    }

    int GetWindowHeight() {
        int width, height;
        glfwGetWindowSize(g_window, &width, &height);
        return height;
    }

    void SwapBuffersPollEvents() {
        glfwSwapBuffers(g_window);
        glfwPollEvents();
    }

    bool WindowIsOpen() {
        return !glfwWindowShouldClose(g_window);
    }

    void AllocateTextureMemory(Texture& texture) {
        OpenGLTexture& glTexture = texture.GetGLTexture();
        GLuint& handle = glTexture.GetHandle();
        if (handle != 0) {
            return; // Perhaps handle this better, or be more descriptive in function name!
        }
        glGenTextures(1, &handle);
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, OpenGLUtil::TextureWrapModeToGLEnum(texture.GetTextureWrapMode()));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, OpenGLUtil::TextureWrapModeToGLEnum(texture.GetTextureWrapMode()));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, OpenGLUtil::TextureFilterToGLEnum(texture.GetMinFilter()));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, OpenGLUtil::TextureFilterToGLEnum(texture.GetMagFilter()));
        int mipmapWidth = texture.GetWidth(0);
        int mipmapHeight = texture.GetHeight(0);
        int levelCount = texture.MipmapsAreRequested() ? texture.GetMipmapLevelCount() : 1;
        for (int i = 0; i < levelCount; i++) {
            if (texture.GetImageDataType() == ImageDataType::UNCOMPRESSED) {
                glTexImage2D(GL_TEXTURE_2D, i, texture.GetInternalFormat(), mipmapWidth, mipmapHeight, 0, texture.GetFormat(), GL_UNSIGNED_BYTE, nullptr);
            }
            if (texture.GetImageDataType() == ImageDataType::COMPRESSED) {
                glCompressedTexImage2D(GL_TEXTURE_2D, i, texture.GetInternalFormat(), mipmapWidth, mipmapHeight, 0, texture.GetDataSize(i), nullptr);
            }
            if (texture.GetImageDataType() == ImageDataType::EXR) {
                // TODO! glTexImage2D(GL_TEXTURE_2D, i, GL_RGB16, mipmapWidth, mipmapHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
            }
            mipmapWidth = std::max(1, mipmapWidth / 2);
            mipmapHeight = std::max(1, mipmapHeight / 2);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void ImmediateBake(QueuedTextureBake& queuedTextureBake) {
        Texture* texture = static_cast<Texture*>(queuedTextureBake.texture);
        OpenGLTexture& glTexture = texture->GetGLTexture();
        int width = queuedTextureBake.width;
        int height = queuedTextureBake.height;
        int format = queuedTextureBake.format;
        int internalFormat = queuedTextureBake.internalFormat;
        int level = queuedTextureBake.mipmapLevel;
        int dataSize = queuedTextureBake.dataSize;
        const void* data = queuedTextureBake.data;

        // Bake texture data
        glBindTexture(GL_TEXTURE_2D, glTexture.GetHandle());
        if (texture->GetImageDataType() == ImageDataType::UNCOMPRESSED) {
            glTexImage2D(GL_TEXTURE_2D, level, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        }
        else if (texture->GetImageDataType() == ImageDataType::EXR) {
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, glTexture.GetWidth(), glTexture.GetHeight(), 0, GL_RGBA, GL_FLOAT, glTexture.GetData());
        }
        else if (texture->GetImageDataType() == ImageDataType::COMPRESSED) {
            glCompressedTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, width, height, internalFormat, dataSize, data);
        }
        texture->SetTextureDataLevelBakeState(level, BakeState::BAKE_COMPLETE);

        // Generate Mipmaps if none were supplied
        if (texture->MipmapsAreRequested()) {
            if (texture->GetTextureDataCount() == 1) {
                glGenerateMipmap(GL_TEXTURE_2D);
            }
        }
        // Cleanup
        glBindTexture(GL_TEXTURE_2D, 0);
        BakeQueue::RemoveQueuedTextureBakeByJobID(queuedTextureBake.jobID);
    }

    void UpdateTextureBaking() {
        for (int i = 0; i < g_textureBakingPBOs.size(); i++) {
            // Update pbo states
            for (PBO& pbo : g_textureBakingPBOs) {
                pbo.UpdateState();
            }
            // If any have completed, remove the job ID from the queue
            for (PBO& pbo : g_textureBakingPBOs) {
                uint32_t jobID = pbo.GetCustomValue();
                if (pbo.IsSyncComplete() && jobID != -1) {
                    QueuedTextureBake* queuedTextureBake = BakeQueue::GetQueuedTextureBakeByJobID(jobID);
                    Texture* texture = static_cast<Texture*>(queuedTextureBake->texture);
                    texture->SetTextureDataLevelBakeState(queuedTextureBake->mipmapLevel, BakeState::BAKE_COMPLETE);
                    // Generate Mipmaps if none were supplied
                    if (texture->MipmapsAreRequested()) {
                        if (texture->GetTextureDataCount() == 1) {
                            glBindTexture(GL_TEXTURE_2D, texture->GetGLTexture().GetHandle());
                            glGenerateMipmap(GL_TEXTURE_2D); 
                            glBindTexture(GL_TEXTURE_2D, 0);
                        }
                    }
                    BakeQueue::RemoveQueuedTextureBakeByJobID(jobID);
                    pbo.SetCustomValue(-1);
                }
            }
            // Bake next queued bake (should one exist)
            if (BakeQueue::GetQueuedTextureBakeJobCount() > 0) {
                QueuedTextureBake* queuedTextureBake = BakeQueue::GetNextQueuedTextureBake();
                if (queuedTextureBake) {
                    AsyncBakeQueuedTextureBake(*queuedTextureBake);
                }
            }
        }
    }

    void AsyncBakeQueuedTextureBake(QueuedTextureBake& queuedTextureBake) {
        // Get next free PBO
        PBO* pbo = nullptr;
        for (PBO& queryPbo : g_textureBakingPBOs) {
            if (queryPbo.IsSyncComplete()) {
                pbo = &queryPbo;
                break;
            }
        }
        // Return early if no free pbos
        if (!pbo) {
            //std::cout << "OpenGLBackend::UploadTexture() return early because there were no pbos avaliable!\n";
            return;
        }
        queuedTextureBake.inProgress = true;

        Texture* texture = static_cast<Texture*>(queuedTextureBake.texture);
        int jobID = queuedTextureBake.jobID;
        int width = queuedTextureBake.width;
        int height = queuedTextureBake.height;
        int format = queuedTextureBake.format;
        int internalFormat = queuedTextureBake.internalFormat;
        int level = queuedTextureBake.mipmapLevel;
        int dataSize = queuedTextureBake.dataSize;
        const void* data = queuedTextureBake.data;

        texture->SetTextureDataLevelBakeState(level, BakeState::BAKING_IN_PROGRESS);
        
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo->GetHandle());
        std::memcpy(pbo->GetPersistentBuffer(), data, dataSize);
        glBindTexture(GL_TEXTURE_2D, texture->GetGLTexture().GetHandle());
        
        if (texture->GetImageDataType() == ImageDataType::UNCOMPRESSED) {
            glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, width, height, format, GL_UNSIGNED_BYTE, nullptr);
        }
        if (texture->GetImageDataType() == ImageDataType::COMPRESSED) {
            glCompressedTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, width, height, internalFormat, dataSize, nullptr);
        }
        if (texture->GetImageDataType() == ImageDataType::EXR) {
            //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, glTexture.GetWidth(), glTexture.GetHeight(), glTexture.GetFormat(), GL_FLOAT, nullptr);
        }
        pbo->SyncStart();
        pbo->SetCustomValue(jobID);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    void CleanUpBakingPBOs() {
        for (PBO& pbo : g_textureBakingPBOs) {
            pbo.CleanUp();
        }
        g_textureBakingPBOs.clear();
    }
}