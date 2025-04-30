#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "AssetManagement/AssetManager.h"
#include "API/OpenGL/GL_backend.h"
#include "API/OpenGL/GL_renderer.h"
#include "Core/Audio.h"
#include "Core/Scene.hpp"
#include "Core/Camera.h"
#include "Input/Input.h"
#include "File/File.h"
#include "TextBlitting/Textblitter.h"
#include "Tools/ImageTools.h"
#include "HellTypes.h"
#include "Timer.hpp"

void Init(const std::string& title) {
    OpenGLBackend::Init(title);
    AssetManager::Init();
    TextBlitter::Init();
    Audio::Init();
    Scene::Init();
    Input::Init(OpenGLBackend::GetWindowPtr());
    Camera::Init(OpenGLBackend::GetWindowPtr());
    Scene::CreateGameObjects();
    OpenGLRenderer::Init();
}

void Update() {
    static float deltaTime = 0;
    static double lastTime = glfwGetTime();
    double currentTime = glfwGetTime();
    deltaTime = static_cast<float>(currentTime - lastTime);
    lastTime = currentTime;
    OpenGLBackend::UpdateTextureBaking();
    Scene::SetMaterials();
    AssetManager::Update();
    TextBlitter::Update();
    Input::Update();
    Camera::Update(deltaTime);
    Audio::Update();
    Scene::Update(deltaTime);
    if (Input::KeyPressed(HELL_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(OpenGLBackend::GetWindowPtr(), true);
    }
    if (Input::KeyPressed(HELL_KEY_F)) {
        OpenGLBackend::ToggleFullscreen();
    }
    if (Input::KeyPressed(HELL_KEY_H)) {
        OpenGLRenderer::LoadShaders();
    }
}

void Render() {
    OpenGLRenderer::RenderFrame();
}

double perform_cpu_intensive_work(uint64_t outerIterations = 100, uint64_t innerIterations = 1000000) {
    volatile double result = 0.0; // Use volatile to hint against over-optimization

    for (uint64_t i = 0; i < outerIterations; ++i) {
        for (uint64_t j = 0; j < innerIterations; ++j) {
            // Perform some arbitrary math operations
            result += std::sqrt(static_cast<double>(i * j + 1));
            result = std::sin(result);
            // Add more operations if needed
        }
    }
    return result; // Return the dummy result
}

int main() {
    Init("GL Depth Peeling");
    glfwSwapInterval(1);

    while (OpenGLBackend::WindowIsOpen()) {
        Update();
        Render();
    }
    glfwTerminate();
    return 0;
}