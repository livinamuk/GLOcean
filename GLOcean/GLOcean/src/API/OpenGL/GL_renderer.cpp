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
#include "Timer.hpp"

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
        Shader oceanUpdateTextures;
        Shader oceanSurfaceComposite;
        Shader underwaterTest;

        Shader ftt_radix_a;
        Shader ftt_radix_b;
        Shader ftt_radix_c;
        Shader ftt_radix_d;
        Shader ftt_radix_e;

    } g_shaders;

    struct FrameBuffers {
        OpenGLFrameBuffer main;
        OpenGLFrameBuffer downSamplesQuarter;
        OpenGLFrameBuffer finalImage;
        OpenGLFrameBuffer hair;
        OpenGLFrameBuffer fft_band0;
        OpenGLFrameBuffer fft_band1;
        OpenGLFrameBuffer water;
    } g_frameBuffers;


    Skybox g_skybox;
    OpenGLMeshPatch g_tesselationPatch;

    OpenGLSSBO g_fftH0band0SSBO;
    OpenGLSSBO g_fftH0band1SSBO;
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

    int g_mode = 0;
    float g_globalTime = 50.0f;

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

    void BlitFrameBuffer(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer, const char* srcName, const char* dstName, GLbitfield mask, GLenum filter);
    //void ComputeInverseFFT(GLuint inputHandle, GLuint outputHandle);

    void Init() {
        InitOceanGPUState();

        g_frameBuffers.main.Create("Main", 1920, 1080);
        g_frameBuffers.main.CreateAttachment("Color", GL_RGBA8);
        g_frameBuffers.main.CreateAttachment("WorldPosition", GL_RGBA32F);
        g_frameBuffers.main.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        g_frameBuffers.water.Create("Water", 1920, 1080);
        g_frameBuffers.water.CreateAttachment("Color", GL_RGBA8);
        g_frameBuffers.water.CreateAttachment("UnderwaterMask", GL_R8);
        g_frameBuffers.water.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        g_frameBuffers.downSamplesQuarter.Create("DownSamples", 1920 / 4, 1080 / 4);
        g_frameBuffers.downSamplesQuarter.CreateAttachment("FinalLighting", GL_RGBA8);
        

        float hairDownscaleRatio = 1.0f;
        g_frameBuffers.hair.Create("Hair", g_frameBuffers.main.GetWidth() * hairDownscaleRatio, g_frameBuffers.main.GetHeight() * hairDownscaleRatio);
        g_frameBuffers.hair.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);
        g_frameBuffers.hair.CreateAttachment("Color", GL_RGBA8);
        g_frameBuffers.hair.CreateAttachment("ViewspaceDepth", GL_R32F);
        g_frameBuffers.hair.CreateAttachment("ViewspaceDepthPrevious", GL_R32F);
        g_frameBuffers.hair.CreateAttachment("Composite", GL_RGBA8);

        g_frameBuffers.fft_band0.Create("FFT", Ocean::GetFFTResolution(0).x, Ocean::GetFFTResolution(0).y);
        g_frameBuffers.fft_band0.CreateAttachment("Displacement", GL_RGBA32F, GL_LINEAR, GL_LINEAR, GL_REPEAT);
        g_frameBuffers.fft_band0.CreateAttachment("Normals", GL_RGBA32F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, true);

        g_frameBuffers.fft_band1.Create("FFT2", Ocean::GetFFTResolution(1).x, Ocean::GetFFTResolution(1).y);
        g_frameBuffers.fft_band1.CreateAttachment("Displacement", GL_RGBA32F, GL_LINEAR, GL_LINEAR, GL_REPEAT, true);
        g_frameBuffers.fft_band1.CreateAttachment("Normals", GL_RGBA32F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, true);

        g_frameBuffers.finalImage.Create("Main", g_frameBuffers.main.GetWidth() * 0.5f, g_frameBuffers.main.GetHeight() * 0.5f);
        g_frameBuffers.finalImage.CreateAttachment("Color", GL_RGBA8);

        LoadShaders();

        g_skybox.Init();
    }

    void RenderFrame() {


        static int band = 1;


        if (Input::KeyPressed(HELL_KEY_1)) {
            g_mode = 1;
            band = 0;
        }
        if (Input::KeyPressed(HELL_KEY_2)) {
            g_mode = 2;
            band = 1;
        }
        if (Input::KeyPressed(HELL_KEY_3)) {
            g_mode = 0;
        }

        bool recompute = false;
        FFTBand& fftBand = Ocean::GetFFTBandByIndex(band);
        if (Input::KeyPressed(HELL_KEY_LEFT)) {
            fftBand.amplitude -= 0.000001;
            recompute = true;
        }
        if (Input::KeyPressed(HELL_KEY_RIGHT)) {
            fftBand.amplitude += 0.000001;
            recompute = true;
        }
        if (Input::KeyPressed(HELL_KEY_NUMPAD_7)) {
            fftBand.patchSimSize -= 10;
            recompute = true;
        }
        if (Input::KeyPressed(HELL_KEY_NUMPAD_9)) {
            fftBand.patchSimSize += 10;
            recompute = true;
        }
        if (Input::KeyDown(HELL_KEY_NUMPAD_4)) {
            fftBand.crossWindDampingCoefficient -= 0.001;
            recompute = true;
        }
        if (Input::KeyDown(HELL_KEY_NUMPAD_6)) {
            fftBand.crossWindDampingCoefficient += 0.001;
            recompute = true;
        }
        if (Input::KeyDown(HELL_KEY_NUMPAD_1)) {
            fftBand.smallWavesDampingCoefficient -= 0.0000001f;
            recompute = true;
        }
        if (Input::KeyDown(HELL_KEY_NUMPAD_3)) {
            fftBand.smallWavesDampingCoefficient += 0.0000001f;
            recompute = true;
        }
        if (recompute) {
            Ocean::ReComputeH0();
            const std::vector<std::complex<float>>& h0Band0 = Ocean::GetH0(0);
            const std::vector<std::complex<float>>& h0Band1 = Ocean::GetH0(1);
            g_fftH0band0SSBO.copyFrom(h0Band0.data(), sizeof(std::complex<float>) * h0Band0.size());
            g_fftH0band1SSBO.copyFrom(h0Band1.data(), sizeof(std::complex<float>) * h0Band1.size());
        }

        //std::cout << "RenderFrame()\n";

        g_frameBuffers.water.Bind();
        g_frameBuffers.water.SetViewport();
        g_frameBuffers.water.DrawBuffers({ "Color", "UnderwaterMask" });
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        g_frameBuffers.main.Bind();
        g_frameBuffers.main.SetViewport();
        g_frameBuffers.main.DrawBuffers({ "Color", "WorldPosition" });
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        //static bool init = false;
        //if (Input::KeyPressed(HELL_KEY_P)) {
        //    init = true;
        //}
        //if (init) {
        //    ComputeOceanFFT();
        //}

        ComputeOceanFFT();
        RenderSkyBox();
        RenderLighting();


        BlitFrameBuffer(&g_frameBuffers.main, &g_frameBuffers.downSamplesQuarter, "Color", "FinalLighting", GL_COLOR_BUFFER_BIT, GL_LINEAR);

        RenderOcean();


        RenderHair();

        // Hair composite
        OpenGLFrameBuffer& mainFrameBuffer = g_frameBuffers.main;
        OpenGLFrameBuffer& hairFrameBuffer = g_frameBuffers.hair;
        g_shaders.hairfinalComposite.Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hairFrameBuffer.GetColorAttachmentHandleByName("Composite"));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mainFrameBuffer.GetColorAttachmentHandleByName("Color"));
        glBindImageTexture(0, mainFrameBuffer.GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glDispatchCompute((g_frameBuffers.main.GetWidth() + 7) / 8, (g_frameBuffers.main.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        UnderwaterTest();
        RenderDebug();

        int width, height;
        glfwGetWindowSize(OpenGLBackend::GetWindowPtr(), &width, &height);


      

        RenderText();


        //g_frameBuffers.main.BlitToDefaultFrameBuffer("Color", 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // Downscale blit
        BlitFrameBuffer(&g_frameBuffers.main, &g_frameBuffers.finalImage, "Color", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // Blit to swapchain
        g_frameBuffers.finalImage.BlitToDefaultFrameBuffer("Color", 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);


        static bool showNormals = false;
        if (Input::KeyPressed(HELL_KEY_M)) {
            showNormals = !showNormals;
        }


        height *= 0.5f;
        if (g_mode == 1 && !showNormals) {
            height = g_frameBuffers.fft_band0.GetWidth() * 1.5f;
            g_frameBuffers.fft_band0.BlitToDefaultFrameBuffer("Displacement", 0, 0, height, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        if (g_mode == 2 && !showNormals) {
            height = g_frameBuffers.fft_band0.GetWidth() * 1.5f;
            g_frameBuffers.fft_band1.BlitToDefaultFrameBuffer("Displacement", 0, 0, height, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        if (g_mode == 1 && showNormals) {
            height = g_frameBuffers.fft_band1.GetWidth() * 1.5f;
            g_frameBuffers.fft_band0.BlitToDefaultFrameBuffer("Normals", 0, 0, height, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        if (g_mode == 2 && showNormals) {
            height = g_frameBuffers.fft_band1.GetWidth() * 1.5f;
            g_frameBuffers.fft_band1.BlitToDefaultFrameBuffer("Normals", 0, 0, height, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
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
        float scale = 2.0f;
        std::string text = "Cam pos: " + Util::Vec3ToString(Camera::GetViewPos());
        //text += "\n";
        //text += "\n";
        //text += Ocean::FFTBandToString(0);
        //text += "\n";
        //text += Ocean::FFTBandToString(1);
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

        Ocean::Init();

        g_tesselationPatch.Resize2(Ocean::GetTesslationMeshSize().x, Ocean::GetTesslationMeshSize().y);

        GLbitfield staticFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        GLbitfield dynamicFlags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;

        const glm::uvec2 oceanSize = Ocean::GetBaseFFTResolution();

        g_fftH0band0SSBO.PreAllocate(Ocean::GetFFTResolution(0).x * Ocean::GetFFTResolution(0).y * sizeof(std::complex<float>), staticFlags);
        g_fftH0band1SSBO.PreAllocate(Ocean::GetFFTResolution(1).x * Ocean::GetFFTResolution(1).y * sizeof(std::complex<float>), staticFlags);

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

        // Upload HO
        const std::vector<std::complex<float>>& h0Band0 = Ocean::GetH0(0);
        const std::vector<std::complex<float>>& h0Band1 = Ocean::GetH0(1);
        g_fftH0band0SSBO.copyFrom(h0Band0.data(), sizeof(std::complex<float>) * h0Band0.size());
        g_fftH0band1SSBO.copyFrom(h0Band1.data(), sizeof(std::complex<float>) * h0Band1.size());
    }

    void ComputeOceanFFT() {

     // FFTBand& band0 = Ocean::GetFFTBandByIndex(0);
     // float y = -0.65f;
     // //float size = band0.patchSimSize.x;
     // float size = 30.0f;
     // glm::vec3 p0 = glm::vec3(0, y, 0);
     // glm::vec3 p1 = glm::vec3(size, y, 0);
     // glm::vec3 p2 = glm::vec3(0, y, size);
     // glm::vec3 p3 = glm::vec3(size, y, size);
     // glm::vec3 color = WHITE;
     // DrawLine(p0, p1, color);
     // DrawLine(p0, p2, color);
     // DrawLine(p3, p1, color);
     // DrawLine(p3, p2, color);
     //
     // size = 12.0f;
     // p0 = glm::vec3(0, y, 0);
     // p1 = glm::vec3(size, y, 0);
     // p2 = glm::vec3(0, y, size);
     // p3 = glm::vec3(size, y, size);
     // color = WHITE;
     // DrawLine(p0, p1, color);
     // DrawLine(p0, p2, color);
     // DrawLine(p3, p1, color);
     // DrawLine(p3, p2, color);


        //Timer("ComputeFTT");

        static double lastTime = glfwGetTime();
        double currentTime = glfwGetTime();
        float deltaTime = float(currentTime - lastTime);
        lastTime = currentTime;

        static bool doTime = true;

        if (doTime) {
            g_globalTime += deltaTime;
        }
        else {
            g_globalTime = 50.0f;
        }
        if (Input::KeyPressed(HELL_KEY_T)) {
            doTime = !doTime;
        }
        //std::cout << globalTime << "\n";

        int bandCount = 2;

        const float gravity = Ocean::GetGravity();

        for (int i = 0; i < bandCount; i++) {

            const glm::uvec2 fftResolution = Ocean::GetFFTResolution(i);

            //const glm::uvec2 fftGridSize = Ocean::GetBaseFFTResolution();
            const glm::vec2 patchSimSize = Ocean::GetPatchSimSize(i);

            const GLuint blocksPerSide = 16;
            const GLuint blockSizeX = fftResolution.x / blocksPerSide;
            const GLuint blockSizeY = fftResolution.y / blocksPerSide;

            // Generate spectrum on GPU
            if (i == 0) {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, g_fftH0band0SSBO.GetHandle());
            }
            if (i == 1) {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, g_fftH0band1SSBO.GetHandle());
            }
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, g_fftSpectrumInSSBO.GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_fftDispInXSSBO.GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, g_fftDispZInSSBO.GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, g_fftGradXInSSBO.GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, g_fftGradZInSSBO.GetHandle());

            g_shaders.oceanCalculateSpectrum.Use();
            g_shaders.oceanCalculateSpectrum.SetUvec2("u_fftGridSize", fftResolution);
            g_shaders.oceanCalculateSpectrum.SetVec2("u_patchSimSize", patchSimSize);
            g_shaders.oceanCalculateSpectrum.SetFloat("u_gravity", gravity);
            g_shaders.oceanCalculateSpectrum.SetFloat("u_time", g_globalTime);
            glDispatchCompute(blockSizeX, blockSizeY, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            // Perform FFT
            Ocean::ComputeInverseFFT2D(fftResolution.x, g_fftSpectrumInSSBO.GetHandle(), g_fftSpectrumOutSSBO.GetHandle());
            Ocean::ComputeInverseFFT2D(fftResolution.x, g_fftDispInXSSBO.GetHandle(), g_fftDispXOutSSBO.GetHandle());
            Ocean::ComputeInverseFFT2D(fftResolution.x, g_fftDispZInSSBO.GetHandle(), g_fftDispZOutSSBO.GetHandle());
            Ocean::ComputeInverseFFT2D(fftResolution.x, g_fftGradXInSSBO.GetHandle(), g_fftGradXOutSSBO.GetHandle());
            Ocean::ComputeInverseFFT2D(fftResolution.x, g_fftGradZInSSBO.GetHandle(), g_fftGradZOutSSBO.GetHandle());

            // Update mesh position
            if (i == 0) {
                glBindImageTexture(0, g_frameBuffers.fft_band0.GetColorAttachmentHandleByName("Displacement"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
                glBindImageTexture(1, g_frameBuffers.fft_band0.GetColorAttachmentHandleByName("Normals"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            }
            if (i == 1) {
                glBindImageTexture(0, g_frameBuffers.fft_band1.GetColorAttachmentHandleByName("Displacement"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
                glBindImageTexture(1, g_frameBuffers.fft_band1.GetColorAttachmentHandleByName("Normals"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            }
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, g_fftSpectrumOutSSBO.GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, g_fftDispXOutSSBO.GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_fftDispZOutSSBO.GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, g_fftGradXOutSSBO.GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, g_fftGradZOutSSBO.GetHandle());
            g_shaders.oceanUpdateTextures.Use();
            g_shaders.oceanUpdateTextures.SetUvec2("u_fftGridSize", fftResolution);
            g_shaders.oceanUpdateTextures.SetFloat("u_dispScale", Ocean::GetDisplacementScale());
            g_shaders.oceanUpdateTextures.SetFloat("u_heightScale", Ocean::GetHeightScale());

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            glDispatchCompute(blockSizeX, blockSizeY, 1);
        }
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

        float scale = 0.05;// Ocean::GetModelMatrixScale();

        int min = -20;
        int max = 20;
        float offset = (max - min) * Ocean::GetBaseFFTResolution().x * scale;

        if (test) {
            min = 0;
            max = 1;
            offset = Ocean::GetBaseFFTResolution().x * scale;
        }

        glm::mat4 projectionMatrix = Camera::GetProjectionMatrix();
        glm::mat4 viewMatrix = Camera::GetViewMatrix();
        glm::vec3 viewPos = Camera::GetViewPos();
        glm::mat4 projectionView = projectionMatrix * viewMatrix;

        float patchOffset = Ocean::GetBaseFFTResolution().y * scale;


        DrawPoint(glm::vec3(0, -0.65f, 0), WHITE);
        DrawPoint(glm::vec3(patchOffset, -0.65f, 0), WHITE);

        Transform tesseleationTransform;
        tesseleationTransform.scale = glm::vec3(scale);

        CopyDepthBuffer(g_frameBuffers.main, g_frameBuffers.water);

        g_frameBuffers.water.Bind();
        g_frameBuffers.water.SetViewport();
        g_frameBuffers.water.DrawBuffers({ "Color", "UnderwaterMask" });
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.fft_band0.GetColorAttachmentHandleByName("Displacement"));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.fft_band0.GetColorAttachmentHandleByName("Normals"));
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.fft_band1.GetColorAttachmentHandleByName("Displacement"));
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.fft_band1.GetColorAttachmentHandleByName("Normals"));
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_CUBE_MAP, g_skybox.cubemap.ID);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.main.GetColorAttachmentHandleByName("WorldPosition"));

        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        // Tessellated ocean
        tesseleationTransform.position.x = -patchOffset;
        g_shaders.oceanGeometry.Use();
        g_shaders.oceanGeometry.SetMat4("u_projectionView", projectionView);
        g_shaders.oceanGeometry.SetVec3("u_wireframeColor", GREEN);
        g_shaders.oceanGeometry.SetMat4("u_model", tesseleationTransform.to_mat4());
        g_shaders.oceanGeometry.SetInt("u_mode", g_mode);
        g_shaders.oceanGeometry.SetVec3("u_viewPos", viewPos);
        g_shaders.oceanGeometry.SetVec2("u_fftGridSize", Ocean::GetBaseFFTResolution());
        g_shaders.oceanGeometry.SetBool("u_wireframe", wireframe);
        g_shaders.oceanGeometry.SetFloat("u_meshSubdivisionFactor", Ocean::GetMeshSubdivisionFactor());

        glGenerateTextureMipmap(g_frameBuffers.fft_band0.GetColorAttachmentHandleByName("Normals"));
        glGenerateTextureMipmap(g_frameBuffers.fft_band1.GetColorAttachmentHandleByName("Normals"));

        glBindVertexArray(g_tesselationPatch.GetVAO());
        glPatchParameteri(GL_PATCH_VERTICES, 4);

        // Surface
        glCullFace(GL_BACK);
        g_shaders.oceanGeometry.SetInt("u_normalMultipler", 1);
        for (int x = min; x < max; x++) {
            for (int z = min; z < max; z++) {
                tesseleationTransform.position = glm::vec3(patchOffset * x, Ocean::GetOceanOriginY(), patchOffset* z);
                if (swap) {
                    tesseleationTransform.position += glm::vec3(offset, 0.0f, 0.0f);
                }
                g_shaders.oceanGeometry.SetMat4("u_model", tesseleationTransform.to_mat4());
                glDrawElements(GL_PATCHES, g_tesselationPatch.GetIndexCount(), GL_UNSIGNED_INT, nullptr);
            }
        }

        // Inverted surface
        glCullFace(GL_FRONT);
        g_shaders.oceanGeometry.SetInt("u_normalMultipler", -1);
        for (int x = min; x < max; x++) {
            for (int z = min; z < max; z++) {
                tesseleationTransform.position = glm::vec3(patchOffset * x, Ocean::GetOceanOriginY(), patchOffset * z);
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
        glCullFace(GL_BACK);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Composite
        glm::mat4 inverseProjectionView = glm::inverse(projectionView);
        glm::vec2 resolution = glm::vec2(g_frameBuffers.main.GetWidth(), g_frameBuffers.main.GetHeight());
        g_shaders.oceanSurfaceComposite.Use();
        g_shaders.oceanSurfaceComposite.SetFloat("u_time", g_globalTime);
        g_shaders.oceanSurfaceComposite.SetVec3("u_viewPos", Camera::GetViewPos());
        g_shaders.oceanSurfaceComposite.SetVec2("u_resolution", resolution);
        g_shaders.oceanSurfaceComposite.SetMat4("u_inverseProjectionView", inverseProjectionView);
        glBindImageTexture(0, g_frameBuffers.main.GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.water.GetColorAttachmentHandleByName("Color"));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.hair.GetColorAttachmentHandleByName("Composite"));
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("WaterNormals")->GetGLTexture().GetHandle());
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("WaterDUDV")->GetGLTexture().GetHandle());
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.water.GetColorAttachmentHandleByName("UnderwaterMask"));
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.downSamplesQuarter.GetColorAttachmentHandleByName("FinalLighting"));


        glDispatchCompute((g_frameBuffers.main.GetWidth() + 7) / 8, (g_frameBuffers.main.GetHeight() + 7) / 8, 1);
    }

    void RenderSkyBox() {

        glEnable(GL_DEPTH_TEST);

        g_frameBuffers.main.Bind();
        g_frameBuffers.main.SetViewport();
        g_frameBuffers.main.DrawBuffers({ "Color", "WorldPosition" });

        Transform skyboxTransform;
        skyboxTransform.position = Camera::GetViewPos();
        skyboxTransform.scale = glm::vec3(200.0f);

        glm::mat4 projectionMatrix = Camera::GetProjectionMatrix();
        glm::mat4 viewMatrix = Camera::GetViewMatrix();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, g_skybox.cubemap.ID);

        g_shaders.skybox.Use();
        g_shaders.skybox.SetInt("environmentMap", 0);
        g_shaders.skybox.SetMat4("view", viewMatrix);
        g_shaders.skybox.SetMat4("projection", projectionMatrix);
        g_shaders.skybox.SetMat4("u_model", skyboxTransform.to_mat4());
        g_shaders.skybox.SetVec3("u_viewPos", Camera::GetViewPos());
        g_shaders.skybox.SetVec3("u_cameraForward", Camera::GetForward());

        glDepthFunc(GL_LEQUAL);
        glBindVertexArray(g_skybox.vao);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glDepthFunc(GL_LESS);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void UnderwaterTest() {
        g_shaders.underwaterTest.Use();
        g_shaders.underwaterTest.SetUvec2("u_fftGridSize", Ocean::GetBaseFFTResolution());
        g_shaders.underwaterTest.SetFloat("u_oceanModelMatrixScale", Ocean::GetModelMatrixScale());
        g_shaders.underwaterTest.SetFloat("u_oceanOriginY", Ocean::GetOceanOriginY());
        g_shaders.underwaterTest.SetInt("u_mode", g_mode);
        g_shaders.underwaterTest.SetVec3("u_viewPos", Camera::GetViewPos());

        glBindImageTexture(0, g_frameBuffers.main.GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.main.GetColorAttachmentHandleByName("WorldPosition"));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.fft_band0.GetColorAttachmentHandleByName("Displacement"));
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, g_frameBuffers.fft_band1.GetColorAttachmentHandleByName("Displacement"));

        glDispatchCompute((g_frameBuffers.main.GetWidth() + 7) / 8, (g_frameBuffers.main.GetHeight() + 7) / 8, 1);
    }

    void LoadShaders() {
        if (
            //g_shaders.ftt_radix_a.Load({ "GL_ftt_radix_a.comp" }) &&
            //g_shaders.ftt_radix_b.Load({ "GL_ftt_radix_b.comp" }) &&
            //g_shaders.ftt_radix_c.Load({ "GL_ftt_radix_c.comp" }) &&
            //g_shaders.ftt_radix_d.Load({ "GL_ftt_radix_d.comp" }) &&
            //g_shaders.ftt_radix_e.Load({ "GL_ftt_radix_e.comp" }) &&
            
            g_shaders.hairfinalComposite.Load({ "gl_hair_final_composite.comp" }) &&
            g_shaders.hairLayerComposite.Load({ "gl_hair_layer_composite.comp" }) &&
            g_shaders.solidColor.Load({ "gl_solid_color.vert", "gl_solid_color.frag" }) &&
            g_shaders.skybox.Load({ "GL_skybox.vert", "GL_skybox.frag" }) &&

            g_shaders.oceanSurfaceComposite.Load({ "GL_ocean_surface_composite.comp" }) &&
            g_shaders.oceanGeometry.Load({ "GL_ocean_geometry.vert", "GL_ocean_geometry.frag" , "GL_ocean_geometry.tesc" , "GL_ocean_geometry.tese" }) &&
            g_shaders.oceanWireframe.Load({ "GL_ocean_wireframe.vert", "GL_ocean_wireframe.frag" }) &&
            g_shaders.oceanCalculateSpectrum.Load({ "GL_ocean_calculate_spectrum.comp" }) &&
            g_shaders.oceanUpdateTextures.Load({ "GL_ocean_update_textures.comp" }) &&
            g_shaders.underwaterTest.Load({ "GL_underwater_test.comp" }) &&

            g_shaders.hairDepthPeel.Load({ "gl_hair_depth_peel.vert", "gl_hair_depth_peel.frag" }) &&
            g_shaders.lighting.Load({ "gl_lighting.vert", "gl_lighting.frag" }) &&
            g_shaders.textBlitter.Load({ "gl_text_blitter.vert", "gl_text_blitter.frag" })) {
            std::cout << "Hotloaded shaders\n";
        }
    }

    void BlitFrameBuffer(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer, const char* srcName, const char* dstName, GLbitfield mask, GLenum filter) {
        GLint srcAttachmentSlot = srcFrameBuffer->GetColorAttachmentSlotByName(srcName);
        GLint dstAttachmentSlot = dstFrameBuffer->GetColorAttachmentSlotByName(dstName);
        if (srcAttachmentSlot != GL_INVALID_VALUE && dstAttachmentSlot != GL_INVALID_VALUE) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFrameBuffer->GetHandle());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFrameBuffer->GetHandle());
            glReadBuffer(srcAttachmentSlot);
            glDrawBuffer(dstAttachmentSlot);
            float srcRectx0 = 0;
            float srcRecty0 = 0;
            float srcRectx1 = srcFrameBuffer->GetWidth();
            float srcRecty1 = srcFrameBuffer->GetHeight();
            float dstRecty0 = 0;
            float dstRectx0 = 0;
            float dstRectx1 = dstFrameBuffer->GetWidth();
            float dstRecty1 = dstFrameBuffer->GetHeight();
            glBlitFramebuffer(srcRectx0, srcRecty0, srcRectx1, srcRecty1, dstRectx0, dstRecty0, dstRectx1, dstRecty1, mask, filter);
        }
    }
}