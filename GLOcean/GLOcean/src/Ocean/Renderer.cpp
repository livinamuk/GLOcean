#include "Renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "Ocean/Renderer.h"
#include "core/GL_mesh_patch.h"
#include "core/MyCamera.h"
#include "core/Input.h"
#include "core/Shader.hpp"
#include "core/MySkybox.hpp"
#include "core/OpenGLBackEnd.h"
#include "core/GL_SSBO.h"
#include "core/Transform.h"

#include "Ocean/Ocean.hpp"

namespace Renderer {

    Shader g_skyboxShader;
    Shader g_oceanGeometryShader;
    Shader g_oceanComputeSpectrumShader;
    Shader g_oceanComputePositionsShader;
    Shader g_oceanComputeNormalsShader;

    Skybox g_skybox;
    OpenGLMeshPatch g_oceanMeshPatch;

    OpenGLSSBO mDevH0;
    OpenGLSSBO mDevGpuSpectrumIn;
    OpenGLSSBO mDevDispXIn;
    OpenGLSSBO mDevDispZIn;
    OpenGLSSBO mDevGradXIn;
    OpenGLSSBO mDevGradZIn;
    OpenGLSSBO mDevGpuSpectrumOut;
    OpenGLSSBO mDevDispXOut;
    OpenGLSSBO mDevDispZOut;
    OpenGLSSBO mDevGradXOut;
    OpenGLSSBO mDevGradZOut;

    void InitOceanGPUState();
    void OceanPass(float deltaTime);

    void HotloadShaders() {
        g_skyboxShader.Load("skybox_env_map_default.vert", "skybox_env_map_default.frag");
        g_oceanGeometryShader.Load("ocean.vert", "ocean.frag");
    }

    void Init() {
        HotloadShaders();
        g_skybox.Init();


        g_oceanMeshPatch.resize(Ocean::GetMeshSize().x, Ocean::GetMeshSize().y);

        InitOceanGPUState();
    }

    void RenderFrame(float deltaTime) {

        if (Input::KeyPressed(HELL_KEY_SPACE)) {

            glm::vec2 dir = glm::normalize(glm::vec2(0.5, 0.9));
            Ocean::SetWindDir(dir);
            std::cout << "speed\n";
            InitOceanGPUState();
        }

        OceanPass(deltaTime);

        const glm::vec3 baseLightDir(0.0f, 1.0f, 0.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 rotatedLightDir = glm::rotateX(baseLightDir, glm::radians(75.0f));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, g_skybox.cubemap.ID);

        glm::mat4 projectionMatrix = MyCamera::GetProjectionMatrix();
        glm::mat4 viewMatrix = MyCamera::GetViewMatrix();
        glm::vec3 viewPos = MyCamera::GetPosition();

        g_oceanGeometryShader.Bind();
        g_oceanGeometryShader.SetInt("environmentMap", 0);
        g_oceanGeometryShader.SetVec3("lightDir", rotatedLightDir);
        g_oceanGeometryShader.SetVec3("eyePos", viewPos);
        g_oceanGeometryShader.SetMat4("view", viewMatrix);
        g_oceanGeometryShader.SetMat4("projection", projectionMatrix);

        Transform t;
        
        float offset = Ocean::GetOceanSize().y;

        int count = 2;
        for (int x = 0; x < count; x++) {
            for (int z = 0; z < 1; z++) {

                t.position = glm::vec3(offset * x, 0, offset * z);
                g_oceanGeometryShader.SetMat4("model", t.to_mat4());
                g_oceanMeshPatch.render();
            }
        }

        // Render skybox
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, g_skybox.cubemap.ID);

        g_skyboxShader.Bind();
        g_skyboxShader.SetInt("environmentMap", 0);
        g_skyboxShader.SetMat4("view", viewMatrix);
        g_skyboxShader.SetMat4("projection", projectionMatrix);

        glDepthFunc(GL_LEQUAL);
        glBindVertexArray(g_skybox.vao);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glDepthFunc(GL_LESS);
    }

    void InitOceanGPUState() {

        GLbitfield staticFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        GLbitfield dynamicFlags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;

        const glm::uvec2 oceanSize = Ocean::GetOceanSize();

        mDevH0.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), staticFlags);
        mDevGpuSpectrumIn.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        mDevGpuSpectrumOut.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        mDevDispXIn.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        mDevDispZIn.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        mDevGradXIn.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        mDevGradZIn.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        mDevDispXOut.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        mDevDispZOut.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        mDevGradXOut.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        mDevGradZOut.PreAllocate(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);

        g_oceanComputeSpectrumShader.Load("ocean_calculate_spectrum.comp");
        g_oceanComputePositionsShader.Load("ocean_update_mesh.comp");
        g_oceanComputeNormalsShader.Load("ocean_update_normals.comp");

        // Precompute HO
        std::vector<std::complex<float>> h0 = Ocean::ComputeH0();
        mDevH0.copyFrom(h0.data(), sizeof(std::complex<float>) * h0.size());
    }

    void OceanPass(float deltaTime) {


        if (Input::KeyPressed(HELL_KEY_H)) {
            g_oceanComputeSpectrumShader.Load("ocean_calculate_spectrum.comp");
            g_oceanComputePositionsShader.Load("ocean_update_mesh.comp");
            g_oceanComputeNormalsShader.Load("ocean_update_normals.comp");
        }

        GLuint h0Handle = mDevH0.GetHandle();

        GLuint spectrumInHandle = mDevGpuSpectrumIn.GetHandle();

        GLuint dispXInHandle = mDevDispXIn.GetHandle();
        GLuint dispZInHandle = mDevDispZIn.GetHandle();
        GLuint dispXOutHandle = mDevDispXOut.GetHandle();
        GLuint dispZOutHandle = mDevDispZOut.GetHandle();

        GLuint gradXInHandle = mDevGradXIn.GetHandle();
        GLuint gradZInHandle = mDevGradZIn.GetHandle();
        GLuint gradXOutHandle = mDevGradXOut.GetHandle();
        GLuint gradZOutHandle = mDevGradZOut.GetHandle();

        const glm::uvec2 oceanSize = Ocean::GetOceanSize();
        const glm::uvec2 meshSize = Ocean::GetMeshSize();
        const glm::vec2 oceanLength = Ocean::GetOceanLength();
        const float gravity = Ocean::GetGravity();
        const float displacementFactor = Ocean::GetDisplacementFactor();

        const GLuint blocksPerSide = 16;
        const GLuint fftBlockSizeX = oceanSize.x / blocksPerSide;
        const GLuint fftBlockSizeY = oceanSize.y / blocksPerSide;
        const GLuint meshBlockSizeX = (meshSize.x + 16 - 1) / blocksPerSide;
        const GLuint meshBlockSizeY = (meshSize.y + 16 - 1) / blocksPerSide;

        // Generate spectrum on GPU
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mDevH0.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mDevGpuSpectrumIn.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mDevDispXIn.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mDevDispZIn.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mDevGradXIn.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, mDevGradZIn.GetHandle());

        g_oceanComputeSpectrumShader.Bind();
        g_oceanComputeSpectrumShader.SetUvec2("oceanSize", oceanSize);
        g_oceanComputeSpectrumShader.SetVec2("oceanLength", oceanLength);
        g_oceanComputeSpectrumShader.SetFloat("g", gravity);
        g_oceanComputeSpectrumShader.SetFloat("t", deltaTime);
        glDispatchCompute(fftBlockSizeX, fftBlockSizeY, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Perform FFT
        Ocean::ComputeInverseFFT2D(mDevGpuSpectrumIn.GetHandle(), mDevGpuSpectrumOut.GetHandle());
        Ocean::ComputeInverseFFT2D(mDevDispXIn.GetHandle(), mDevDispXOut.GetHandle());
        Ocean::ComputeInverseFFT2D(mDevDispZIn.GetHandle(), mDevDispZOut.GetHandle());
        Ocean::ComputeInverseFFT2D(mDevGradXIn.GetHandle(), mDevGradXOut.GetHandle());
        Ocean::ComputeInverseFFT2D(mDevGradZIn.GetHandle(), mDevGradZOut.GetHandle());

        // Update mesh position
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mDevGpuSpectrumOut.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mDevDispXOut.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mDevDispZOut.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, g_oceanMeshPatch.GetVBO());
        g_oceanComputePositionsShader.Bind();
        g_oceanComputePositionsShader.SetUvec2("oceanSize", oceanSize);
        g_oceanComputePositionsShader.SetUvec2("meshSize", meshSize);
        g_oceanComputePositionsShader.SetFloat("dispFactor", displacementFactor);
        glDispatchCompute(meshBlockSizeX, meshBlockSizeY, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Update normals
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mDevGpuSpectrumOut.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mDevGradXOut.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mDevGradZOut.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, g_oceanMeshPatch.GetVBO());
        g_oceanComputeNormalsShader.Bind();
        g_oceanComputeNormalsShader.SetUvec2("oceanSize", oceanSize);
        g_oceanComputeNormalsShader.SetUvec2("meshSize", meshSize);
        glDispatchCompute(meshBlockSizeX, meshBlockSizeY, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
}