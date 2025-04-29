#include "GL_renderer.h"
#include "GL_backend.h"
#include "GL_util.hpp"
#include "Types/GL_detachedMesh.hpp"
#include "Types/GL_frameBuffer.h"
#include "Types/GL_mesh_patch.h"
#include "Types/GL_pbo.hpp"
#include "Types/GL_shader.h"
#include "Types/GL_ssbo.h"
#include "../AssetManagement/AssetManager.h"
#include "../Core/Audio.h"
#include "../Core/Camera.h"
#include "../Core/Scene.hpp"
#include "../Input/Input.h"
#include "../Util.hpp"
#include "../TextBlitting/TextBlitter.h"
#include "../Types/GameObject.h"
#include "../Types/Skybox.hpp"
#include "../Hardcoded.hpp"
#include <glm/gtx/matrix_decompose.hpp>

#include "../Ocean/Ocean.h"
#include <glm/gtx/rotate_vector.hpp>

namespace OpenGLRenderer {

    struct Shaders {
        Shader solidColor;
        Shader lighting;
        Shader hairDepthPeel;
        Shader textBlitter;
        Shader hairfinalComposite;
        Shader hairLayerComposite;
        Shader skybox;
        Shader oceanGeometry;
        Shader oceanWireframe;
        Shader oceanCalculateSpectrum;
        Shader oceanUpdateMesh;
        Shader underwaterTest;
    } g_shaders;

    struct FrameBuffers {
        OpenGLFrameBuffer main;
        OpenGLFrameBuffer hair;
        OpenGLFrameBuffer fft;
    } g_frameBuffers;


    Skybox g_skybox;
    OpenGLMeshPatch g_tesselationPatch;

    OpenGLSSBO g_fftH0SSBO;
    OpenGLSSBO g_fftSpectrumInSSBO;
    OpenGLSSBO g_fftSpectrumOutSSBO;
    OpenGLSSBO g_fftDispInXSSBO;
    OpenGLSSBO g_fftDispZInSSBO;
    OpenGLSSBO g_fftGradXInSSBO;
    OpenGLSSBO g_fftGradZInSSBO;
    OpenGLSSBO g_fftDispXOutSSBO;
    OpenGLSSBO g_fftDispZOutSSBO;
    OpenGLSSBO g_fftGradXOutSSBO;
    OpenGLSSBO g_fftGradZOutSSBO;

    void InitOceanGPUState();
    void DrawScene(Shader& shader);
    void ComputeOceanFFT();
    void RenderOcean();
    void RenderLighting();
    void RenderDebug();
    void RenderHair();
    void RenderHairLayer(std::vector<RenderItem>& renderItems, int peelCount);
    void RenderText();
    void RenderSkyBox();
    void UnderwaterTest();

    void Init() {
        g_frameBuffers.main.Create("Main", 1920, 1080);
        g_frameBuffers.main.CreateAttachment("Color", GL_RGBA8);
        g_frameBuffers.main.CreateAttachment("WorldPosition", GL_RGBA32F);
        g_frameBuffers.main.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        float hairDownscaleRatio = 1.0f;
        g_frameBuffers.hair.Create("Hair", g_frameBuffers.main.GetWidth() * hairDownscaleRatio, g_frameBuffers.main.GetHeight() * hairDownscaleRatio);
        g_frameBuffers.hair.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);
        g_frameBuffers.hair.CreateAttachment("Color", GL_RGBA8);
        g_frameBuffers.hair.CreateAttachment("ViewspaceDepth", GL_R32F);
        g_frameBuffers.hair.CreateAttachment("ViewspaceDepthPrevious", GL_R32F);
        g_frameBuffers.hair.CreateAttachment("Composite", GL_RGBA8);

        g_frameBuffers.fft.Create("FFT", Ocean::GetFFTGridSize().x, Ocean::GetFFTGridSize().y);
        g_frameBuffers.fft.CreateAttachment("Height", GL_R32F, GL_NEAREST, GL_REPEAT);
        g_frameBuffers.fft.CreateAttachment("DisplacementX", GL_R32F, GL_NEAREST, GL_REPEAT);
        g_frameBuffers.fft.CreateAttachment("DisplacementZ", GL_R32F, GL_NEAREST, GL_REPEAT);
        g_frameBuffers.fft.CreateAttachment("WorldPosition", GL_RGBA32F, GL_NEAREST, GL_REPEAT);
        g_frameBuffers.fft.CreateAttachment("Normals", GL_RGBA32F, GL_NEAREST, GL_REPEAT);

        LoadShaders();

        InitOceanGPUState();

        g_skybox.Init();
    }

    void RenderFrame() {

        glfwSwapInterval(1);

        g_frameBuffers.fft.Bind();
        g_frameBuffers.fft.SetViewport();
        g_frameBuffers.fft.DrawBuffers({ "Height" });
        glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        g_frameBuffers.main.Bind();
        g_frameBuffers.main.SetViewport();
        g_frameBuffers.main.DrawBuffers({ "Color", "WorldPosition" });
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ComputeOceanFFT();

        RenderLighting();
        RenderSkyBox();
        RenderHair();
        RenderOcean();
        UnderwaterTest();
        RenderDebug();

        int width, height;
        glfwGetWindowSize(OpenGLBackend::GetWindowPtr(), &width, &height);

        OpenGLFrameBuffer& mainFrameBuffer = g_frameBuffers.main;
        OpenGLFrameBuffer& hairFrameBuffer = g_frameBuffers.hair;

        g_shaders.hairfinalComposite.Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hairFrameBuffer.GetColorAttachmentHandleByName("Composite"));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mainFrameBuffer.GetColorAttachmentHandleByName("Color"));
        glBindImageTexture(0, mainFrameBuffer.GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glDispatchCompute((g_frameBuffers.main.GetWidth() + 7) / 8, (g_frameBuffers.main.GetHeight() + 7) / 8, 1);

        RenderText();


        g_frameBuffers.main.BlitToDefaultFrameBuffer("Color", 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        static int debugMode = 0;
        if (Input::KeyPressed(HELL_KEY_M)) {
            debugMode++;
            if (debugMode == 6) {
                debugMode = 0;
            }
        }

        height *= 0.5f;
        if (debugMode == 1) {
            g_frameBuffers.fft.BlitToDefaultFrameBuffer("Height", 0, 0, height, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        if (debugMode == 2) {
            g_frameBuffers.fft.BlitToDefaultFrameBuffer("DisplacementX", 0, 0, height, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        if (debugMode == 3) {
            g_frameBuffers.fft.BlitToDefaultFrameBuffer("DisplacementZ", 0, 0, height, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        if (debugMode == 4) {
            g_frameBuffers.fft.BlitToDefaultFrameBuffer("WorldPosition", 0, 0, height, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        if (debugMode == 5) {
            g_frameBuffers.fft.BlitToDefaultFrameBuffer("Normals", 0, 0, height, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }

        glfwSwapBuffers(OpenGLBackend::GetWindowPtr());
        glfwPollEvents();
    }

    void CopyDepthBuffer(OpenGLFrameBuffer& srcFrameBuffer, OpenGLFrameBuffer& dstFrameBuffer) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFrameBuffer.GetHandle());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFrameBuffer.GetHandle());
        glBlitFramebuffer(0, 0, srcFrameBuffer.GetWidth(), srcFrameBuffer.GetHeight(), 0, 0, dstFrameBuffer.GetWidth(), dstFrameBuffer.GetHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    void CopyColorBuffer(OpenGLFrameBuffer& srcFrameBuffer, OpenGLFrameBuffer& dstFrameBuffer, const char* srcAttachmentName, const char* dstAttachmentName) {
        GLenum srcAttachmentSlot = srcFrameBuffer.GetColorAttachmentSlotByName(srcAttachmentName);
        GLenum dstAttachmentSlot = dstFrameBuffer.GetColorAttachmentSlotByName(dstAttachmentName);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFrameBuffer.GetHandle());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFrameBuffer.GetHandle());
        glReadBuffer(srcAttachmentSlot);
        glDrawBuffer(dstAttachmentSlot);
        glBlitFramebuffer(0, 0, srcFrameBuffer.GetWidth(), srcFrameBuffer.GetHeight(), 0, 0, dstFrameBuffer.GetWidth(), dstFrameBuffer.GetHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    void DrawScene(Shader& shader) {
        // Non blended
        for (RenderItem& renderItem : Scene::GetRenderItems()) {
            OpenGLDetachedMesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
            if (mesh) {
                shader.SetMat4("model", renderItem.modelMatrix);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.normalTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());
                glBindVertexArray(mesh->GetVAO());
                glDrawElements(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT, 0);
            }
        }
        // Blended
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        for (RenderItem& renderItem : Scene::GetRenderItemsBlended()) {
            OpenGLDetachedMesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
            if (mesh) {
                shader.SetMat4("model", renderItem.modelMatrix);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.normalTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());
                glBindVertexArray(mesh->GetVAO());
                glDrawElements(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT, 0);
            }
        }
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }


    void RenderLighting() {
        const float waterHeight = Hardcoded::roomY + Hardcoded::waterHeight;
        static float time = 0;
        time += 1.0f / 60.0f;

        g_frameBuffers.main.Bind();
        g_frameBuffers.main.SetViewport();
        g_frameBuffers.main.DrawBuffers({ "Color", "WorldPosition" });

        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        g_shaders.lighting.Use();
        g_shaders.lighting.SetMat4("projection", Camera::GetProjectionMatrix());
        g_shaders.lighting.SetMat4("view", Camera::GetViewMatrix());
        g_shaders.lighting.SetMat4("model", glm::mat4(1));
        g_shaders.lighting.SetVec3("viewPos", Camera::GetViewPos());
        g_shaders.lighting.SetFloat("time", time);
        g_shaders.lighting.SetFloat("viewportWidth", g_frameBuffers.hair.GetWidth());
        g_shaders.lighting.SetFloat("viewportHeight", g_frameBuffers.hair.GetHeight());
        DrawScene(g_shaders.lighting);
    }



    void RenderHair() {
        OpenGLFrameBuffer& mainFrameBuffer = g_frameBuffers.main;
        OpenGLFrameBuffer& hairFrameBuffer = g_frameBuffers.hair;

        static int peelCount = 4;
        if (Input::KeyPressed(HELL_KEY_8) && peelCount < 7) {
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            peelCount++;
            std::cout << "Depth peel layer count: " << peelCount << "\n";
        }
        if (Input::KeyPressed(HELL_KEY_9) && peelCount > 0) {
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            peelCount--;
            std::cout << "Depth peel layer count: " << peelCount << "\n";
        }
        // Blit debug text
        int viewportWidth = mainFrameBuffer.GetWidth();
        int viewportHeight = mainFrameBuffer.GetHeight();
        int locationX = 0;
        int locationY = 0;
        float scale = 2.5f;
        std::string text = "Cam pos: " + Util::Vec3ToString(Camera::GetViewPos());
        TextBlitter::BlitText(text, "StandardFont", locationX, locationY, viewportWidth, viewportHeight, scale);

        // Setup state
        Shader* shader = &g_shaders.lighting;
        shader->Use();
        shader->SetBool("isHair", true);
        g_frameBuffers.hair.Bind();
        g_frameBuffers.hair.ClearAttachment("Composite", 0, 0, 0, 0);
        g_frameBuffers.hair.SetViewport();
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        // Render all top then all Bottom layers
        RenderHairLayer(Scene::GetRenderItemsHairTopLayer(), peelCount);
        RenderHairLayer(Scene::GetRenderItemsHairBottomLayer(), peelCount);

        g_shaders.hairfinalComposite.Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hairFrameBuffer.GetColorAttachmentHandleByName("Composite"));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mainFrameBuffer.GetColorAttachmentHandleByName("Color"));
        glBindImageTexture(0, mainFrameBuffer.GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glDispatchCompute((g_frameBuffers.main.GetWidth() + 7) / 8, (g_frameBuffers.main.GetHeight() + 7) / 8, 1);

        // Cleanup
        shader->Use();
        shader->SetBool("isHair", false);
        g_frameBuffers.main.SetViewport();
        glDepthFunc(GL_LESS);
    }

    void RenderHairLayer(std::vector<RenderItem>& renderItems, int peelCount) {
        g_frameBuffers.hair.Bind();
        g_frameBuffers.hair.ClearAttachment("ViewspaceDepthPrevious", 1, 1, 1, 1);
        for (int i = 0; i < peelCount; i++) {
            // Viewspace depth pass
            CopyDepthBuffer(g_frameBuffers.main, g_frameBuffers.hair);
            g_frameBuffers.hair.Bind();
            g_frameBuffers.hair.ClearAttachment("ViewspaceDepth", 0, 0, 0, 0);
            g_frameBuffers.hair.DrawBuffer("ViewspaceDepth");
            glDepthFunc(GL_LESS);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, g_frameBuffers.hair.GetColorAttachmentHandleByName("ViewspaceDepthPrevious"));
            Shader* shader = &g_shaders.hairDepthPeel;
            shader->Use();
            shader->SetMat4("projection", Camera::GetProjectionMatrix());
            shader->SetMat4("view", Camera::GetViewMatrix());
            shader->SetFloat("nearPlane", NEAR_PLANE);
            shader->SetFloat("farPlane", FAR_PLANE);
            shader->SetFloat("viewportWidth", g_frameBuffers.hair.GetWidth());
            shader->SetFloat("viewportHeight", g_frameBuffers.hair.GetHeight());
            for (RenderItem& renderItem : renderItems) {
                OpenGLDetachedMesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
                if (mesh) {
                    shader->SetMat4("model", renderItem.modelMatrix);
                    glBindVertexArray(mesh->GetVAO());
                    glDrawElements(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT, 0);
                }
            }
            // Color pass
            glDepthFunc(GL_EQUAL);
            g_frameBuffers.hair.Bind();
            g_frameBuffers.hair.ClearAttachment("Color", 0, 0, 0, 0);
            g_frameBuffers.hair.DrawBuffer("Color");
            shader = &g_shaders.lighting;
            shader->Use();
            shader->SetMat4("projection", Camera::GetProjectionMatrix());
            shader->SetMat4("view", Camera::GetViewMatrix());
            for (RenderItem& renderItem : renderItems) {
                OpenGLDetachedMesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
                if (mesh) {
                    shader->SetMat4("model", renderItem.modelMatrix);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.normalTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());
                    glBindVertexArray(mesh->GetVAO());
                    glDrawElements(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT, 0);
                }
            }
            // TODO!: when you port this to the main engine you can output previous viewspace depth in the pass above
            CopyColorBuffer(g_frameBuffers.hair, g_frameBuffers.hair, "ViewspaceDepth", "ViewspaceDepthPrevious");

            // Composite
            g_shaders.hairLayerComposite.Use();
            glBindImageTexture(0, g_frameBuffers.hair.GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
            glBindImageTexture(1, g_frameBuffers.hair.GetColorAttachmentHandleByName("Composite"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
            glDispatchCompute((g_frameBuffers.hair.GetWidth() + 7) / 8, (g_frameBuffers.hair.GetHeight() + 7) / 8, 1);
        }
    }

    void RenderDebug() {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glPointSize(8.0f);
        glDisable(GL_DEPTH_TEST);

        g_shaders.solidColor.Use();
        g_shaders.solidColor.SetMat4("projection", Camera::GetProjectionMatrix());
        g_shaders.solidColor.SetMat4("view", Camera::GetViewMatrix());
        g_shaders.solidColor.SetMat4("model", glm::mat4(1));

        // Draw lines
        UpdateDebugLinesMesh();
        if (g_debugLinesMesh.GetIndexCount() > 0) {
            glBindVertexArray(g_debugLinesMesh.GetVAO());
            glDrawElements(GL_LINES, g_debugLinesMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
        }
        // Draw points
        UpdateDebugPointsMesh();
        if (g_debugPointsMesh.GetIndexCount() > 0) {
            glBindVertexArray(g_debugPointsMesh.GetVAO());
            glDrawElements(GL_POINTS, g_debugPointsMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
        }
    }

    void RenderText() {
        OpenGLFrameBuffer& mainFrameBuffer = g_frameBuffers.main;
        mainFrameBuffer.Bind();
        mainFrameBuffer.SetViewport();

        Shader& shader = g_shaders.textBlitter;
        shader.Use();
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        OpenGLFontMesh* fontMesh = TextBlitter::GetGLFontMesh("StandardFont");
        if (fontMesh) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("StandardFont")->GetGLTexture().GetHandle());
            glBindVertexArray(fontMesh->GetVAO());
            glDrawElements(GL_TRIANGLES, fontMesh->GetIndexCount(), GL_UNSIGNED_INT, 0);
        }

        // Cleanup
        glDisable(GL_BLEND);
    }

    void InitOceanGPUState() {
        g_tesselationPatch.Resize2(Ocean::GetTesslationMeshSize().x, Ocean::GetTesslationMeshSize().y);

        GLbitfield staticFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        GLbitfield dynamicFlags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;

        const glm::uvec2 oceanSize = Ocean::GetFFTGridSize();

        g_fftH0SSBO.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), staticFlags);
        g_fftSpectrumInSSBO.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_fftSpectrumOutSSBO.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_fftDispInXSSBO.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_fftDispZInSSBO.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_fftGradXInSSBO.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_fftGradZInSSBO.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_fftDispXOutSSBO.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_fftDispZOutSSBO.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_fftGradXOutSSBO.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_fftGradZOutSSBO.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);

        // Precompute HO
        std::vector<std::complex<float>> h0 = Ocean::ComputeH0();
        g_fftH0SSBO.copyFrom(h0.data(), sizeof(std::complex<float>) * h0.size());
    }

    void ComputeOceanFFT() {

        static double lastTime = glfwGetTime();
        double currentTime = glfwGetTime();
        float deltaTime = float(currentTime - lastTime);
        lastTime = currentTime;

        static float globalTime = 0.0f;
        globalTime += deltaTime;

        //globalTime = 0;

        const glm::uvec2 fftGridSize = Ocean::GetFFTGridSize();
        const glm::uvec2 meshSize = Ocean::GetMeshSize();
        const glm::vec2 oceanLength = Ocean::GetOceanLength();
        const float gravity = Ocean::GetGravity();
        const float displacementFactor = Ocean::GetDisplacementFactor();

        const GLuint blocksPerSide = 16;
        const GLuint fftBlockSizeX = fftGridSize.x / blocksPerSide;
        const GLuint fftBlockSizeY = fftGridSize.y / blocksPerSide;
        const GLuint meshBlockSizeX = (meshSize.x + 16 - 1) / blocksPerSide;
        const GLuint meshBlockSizeY = (meshSize.y + 16 - 1) / blocksPerSide;

        // Generate spectrum on GPU
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, g_fftH0SSBO.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, g_fftSpectrumInSSBO.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_fftDispInXSSBO.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, g_fftDispZInSSBO.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, g_fftGradXInSSBO.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, g_fftGradZInSSBO.GetHandle());

        g_shaders.oceanCalculateSpectrum.Use();
        g_shaders.oceanCalculateSpectrum.SetUvec2("oceanSize", fftGridSize);
        g_shaders.oceanCalculateSpectrum.SetVec2("oceanLength", oceanLength);
        g_shaders.oceanCalculateSpectrum.SetFloat("g", gravity);
        g_shaders.oceanCalculateSpectrum.SetFloat("t", globalTime);
        glDispatchCompute(fftBlockSizeX, fftBlockSizeY, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Perform FFT
        Ocean::ComputeInverseFFT2D(g_fftSpectrumInSSBO.GetHandle(), g_fftSpectrumOutSSBO.GetHandle());
        Ocean::ComputeInverseFFT2D(g_fftDispInXSSBO.GetHandle(), g_fftDispXOutSSBO.GetHandle());
        Ocean::ComputeInverseFFT2D(g_fftDispZInSSBO.GetHandle(), g_fftDispZOutSSBO.GetHandle());
        Ocean::ComputeInverseFFT2D(g_fftGradXInSSBO.GetHandle(), g_fftGradXOutSSBO.GetHandle());
        Ocean::ComputeInverseFFT2D(g_fftGradZInSSBO.GetHandle(), g_fftGradZOutSSBO.GetHandle());

        // Update mesh position
        glBindImageTexture(0, g_frameBuffers.fft.GetColorAttachmentHandleByName("Height"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
        glBindImageTexture(1, g_frameBuffers.fft.GetColorAttachmentHandleByName("DisplacementX"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
        glBindImageTexture(2, g_frameBuffers.fft.GetColorAttachmentHandleByName("DisplacementZ"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
        glBindImageTexture(3, g_frameBuffers.fft.GetColorAttachmentHandleByName("WorldPosition"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glBindImageTexture(4, g_frameBuffers.fft.GetColorAttachmentHandleByName("Normals"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, g_fftSpectrumOutSSBO.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, g_fftDispXOutSSBO.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_fftDispZOutSSBO.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, g_fftGradXOutSSBO.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, g_fftGradZOutSSBO.GetHandle());
        g_shaders.oceanUpdateMesh.Use();
        g_shaders.oceanUpdateMesh.SetUvec2("u_fftGridSize", fftGridSize);
        g_shaders.oceanUpdateMesh.SetUvec2("u_meshSize", meshSize);
        g_shaders.oceanUpdateMesh.SetFloat("u_dispFactor", displacementFactor);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glFinish();
        glDispatchCompute(meshBlockSizeX, meshBlockSizeY, 1);
    }

    void CheckGLErrors(const char* context) {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            const char* error = "UNKNOWN";
            switch (err) {
                case GL_INVALID_ENUM:                  error = "GL_INVALID_ENUM"; break;
                case GL_INVALID_VALUE:                 error = "GL_INVALID_VALUE"; break;
                case GL_INVALID_OPERATION:             error = "GL_INVALID_OPERATION"; break;
                case GL_STACK_OVERFLOW:                error = "GL_STACK_OVERFLOW"; break;
                case GL_STACK_UNDERFLOW:                error = "GL_STACK_UNDERFLOW"; break;
                case GL_OUT_OF_MEMORY:                  error = "GL_OUT_OF_MEMORY"; break;
                case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            }
            std::cout << "[OpenGL Error] (" << error << ") at " << context << std::endl;
        }
    }

    void RenderOcean() {
        //CheckGLErrors("start of RenderOcean");

        static bool wireframe = false;
        static bool swap = false;
        static bool test = false;

        if (Input::KeyPressed(HELL_KEY_V)) {
            test = !test;
        }
        if (Input::KeyPressed(HELL_KEY_B)) {
            wireframe = !wireframe;
        }
        if (Input::KeyPressed(HELL_KEY_N)) {
            swap = !swap;
        }

        float scale = 8.0f / Ocean::GetFFTGridSize().x;

        int min = -20;
        int max = 20;
        float offset = (max - min) * Ocean::GetFFTGridSize().x * scale;

        if (test) {
            min = 0;
            max = 1;
            offset = Ocean::GetFFTGridSize().x * scale;
        }

        glm::mat4 projectionMatrix = Camera::GetProjectionMatrix();
        glm::mat4 viewMatrix = Camera::GetViewMatrix();
        glm::vec3 viewPos = Camera::GetViewPos();
        glm::mat4 projectionView = projectionMatrix * viewMatrix;

        float patchOffset = Ocean::GetFFTGridSize().y * scale;

        Transform tesseleationTransform;
        tesseleationTransform.scale = glm::vec3(scale);

        g_frameBuffers.main.Bind();
        g_frameBuffers.main.SetViewport();
        g_frameBuffers.main.DrawBuffers({ "Color" });

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.fft.GetColorAttachmentHandleByName("Height"));

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.fft.GetColorAttachmentHandleByName("DisplacementX"));

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.fft.GetColorAttachmentHandleByName("DisplacementZ"));

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, g_skybox.cubemap.ID);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.fft.GetColorAttachmentHandleByName("WorldPosition"));

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.fft.GetColorAttachmentHandleByName("Normals"));

        if (wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        // Tessellated ocean
        tesseleationTransform.position.x = -patchOffset;
        g_shaders.oceanGeometry.Use();
        g_shaders.oceanGeometry.SetMat4("u_projectionView", projectionView);
        g_shaders.oceanGeometry.SetVec3("u_wireframeColor", GREEN);
        g_shaders.oceanGeometry.SetMat4("u_model", tesseleationTransform.to_mat4());
        g_shaders.oceanGeometry.SetVec3("u_viewPos", viewPos);
        g_shaders.oceanGeometry.SetVec2("u_fftGridSize", Ocean::GetFFTGridSize());
        g_shaders.oceanGeometry.SetBool("u_wireframe", wireframe);
        g_shaders.oceanGeometry.SetFloat("u_meshSubdivisionFactor", Ocean::GetMeshSubdivisionFactor());

        glBindVertexArray(g_tesselationPatch.GetVAO());
        glPatchParameteri(GL_PATCH_VERTICES, 4);
        for (int x = min; x < max; x++) {
            for (int z = min; z < max; z++) {
                tesseleationTransform.position = glm::vec3(patchOffset * x, -0.65f, patchOffset * z);
                if (swap) {
                    tesseleationTransform.position += glm::vec3(offset, 0.0f, 0.0f);
                }
                g_shaders.oceanGeometry.SetMat4("u_model", tesseleationTransform.to_mat4());
                glDrawElements(GL_PATCHES, g_tesselationPatch.GetIndexCount(), GL_UNSIGNED_INT, nullptr);
            }
        }
        // Cleanup
        g_shaders.oceanGeometry.SetBool("u_wireframe", false);
        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    void RenderSkyBox() {
        g_frameBuffers.main.Bind();
        g_frameBuffers.main.SetViewport();
        g_frameBuffers.main.DrawBuffers({ "Color" });

        glm::mat4 projectionMatrix = Camera::GetProjectionMatrix();
        glm::mat4 viewMatrix = Camera::GetViewMatrix();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, g_skybox.cubemap.ID);

        g_shaders.skybox.Use();
        g_shaders.skybox.SetInt("environmentMap", 0);
        g_shaders.skybox.SetMat4("view", viewMatrix);
        g_shaders.skybox.SetMat4("projection", projectionMatrix);

        glDepthFunc(GL_LEQUAL);
        glBindVertexArray(g_skybox.vao);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glDepthFunc(GL_LESS);
    }

    void UnderwaterTest() {
        g_shaders.underwaterTest.Use();

        glBindImageTexture(0, g_frameBuffers.main.GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.main.GetColorAttachmentHandleByName("WorldPosition"));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.fft.GetColorAttachmentHandleByName("Height"));

        glDispatchCompute((g_frameBuffers.main.GetWidth() + 7) / 8, (g_frameBuffers.main.GetHeight() + 7) / 8, 1);
    }

    void LoadShaders() {
        if (g_shaders.hairfinalComposite.Load({ "gl_hair_final_composite.comp" }) &&
            g_shaders.hairLayerComposite.Load({ "gl_hair_layer_composite.comp" }) &&
            g_shaders.solidColor.Load({ "gl_solid_color.vert", "gl_solid_color.frag" }) &&
            g_shaders.skybox.Load({ "GL_skybox.vert", "GL_skybox.frag" }) &&
            g_shaders.oceanGeometry.Load({ "GL_ocean_geometry.vert", "GL_ocean_geometry.frag" , "GL_ocean_geometry.tesc" , "GL_ocean_geometry.tese" }) &&
            g_shaders.oceanWireframe.Load({ "GL_ocean_wireframe.vert", "GL_ocean_wireframe.frag" }) &&
            g_shaders.oceanCalculateSpectrum.Load({ "GL_ocean_calculate_spectrum.comp" }) &&
            g_shaders.oceanUpdateMesh.Load({ "GL_ocean_update_mesh.comp" }) &&
            g_shaders.underwaterTest.Load({ "GL_underwater_test.comp" }) &&
            g_shaders.hairDepthPeel.Load({ "gl_hair_depth_peel.vert", "gl_hair_depth_peel.frag" }) &&
            g_shaders.lighting.Load({ "gl_lighting.vert", "gl_lighting.frag" }) &&
            g_shaders.textBlitter.Load({ "gl_text_blitter.vert", "gl_text_blitter.frag" })) {
            std::cout << "Hotloaded shaders\n";
        }
    }
}