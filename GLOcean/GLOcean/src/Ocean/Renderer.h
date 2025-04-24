#pragma once
#include <glm/glm.hpp>

#include <vector>
#include <complex>
#include <random>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace Renderer {

    void HotloadShaders();

    void Init();
    void RenderFrame(float deltaTime);
    void OceanPass(float deltaTime);

}