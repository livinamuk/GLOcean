#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "HellTypes.h"

#define NEAR_PLANE 0.01f
#define FAR_PLANE 20.0f

namespace Camera {
    void Init(GLFWwindow* window);
    void Update(float deltaTime);
    glm::mat4 GetProjectionMatrix();
    glm::mat4 GetViewMatrix();
    glm::mat4 GetInverseViewMatrix();
    glm::vec3 GetViewPos();
    Transform GetTransform(); 
    glm::vec3 GetViewRotation();
    glm::vec3 GetForward();
    glm::vec3 GetRight();
    glm::vec3 GetUp();
}