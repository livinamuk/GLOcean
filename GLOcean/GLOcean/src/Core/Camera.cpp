#include "Camera.h"
#include "../Input/Input.h"

#define NEAR_PLANE 0.01f
#define FAR_PLANE 5000.0f

namespace Camera {
    Transform g_transform;
    double g_mouseX = 0;
    double g_mouseY = 0;
    double g_mouseOffsetX = 0;
    double g_mouseOffsetY = 0;
    float g_mouseSensitivity = 0.002f;
    float g_walkSpeed = 5.0f;
    GLFWwindow* g_window;

    void Init(GLFWwindow* window) {
        g_window = window;
        //gTransform.position = glm::vec3(0, -0.15f, 0);
        //gTransform.rotation = glm::vec3(-0.25f, 0.0f, 0.0f);
        //g_transform.position = glm::vec3(3.44, 0.55, 6.5);
        //g_transform.rotation = glm::vec3(-0.12, 3.14f, 0.00);
        g_transform.position = glm::vec3(18.0, 0.0, 8);
        g_transform.rotation = glm::vec3(-0.40, 3.34, 0.00);

        double x, y;
        glfwGetCursorPos(g_window, &x, &y);
        g_mouseOffsetX = x;
        g_mouseOffsetY = y;
        g_mouseX = x;
        g_mouseY = y;
    }

    void Update(float deltaTime) {
        // Mouselook
        double x, y;
        glfwGetCursorPos(g_window, &x, &y);
        g_mouseOffsetX = x - g_mouseX;
        g_mouseOffsetY = y - g_mouseY;
        g_mouseX = x;
        g_mouseY = y;
        g_transform.rotation.x += -g_mouseOffsetY * g_mouseSensitivity;
        g_transform.rotation.y += -g_mouseOffsetX * g_mouseSensitivity;
        g_transform.rotation.x = std::min(g_transform.rotation.x, 1.5f);
        g_transform.rotation.x = std::max(g_transform.rotation.x, -1.5f);
        glm::vec3 camRight = glm::vec3(g_transform.to_mat4()[0]);
        glm::vec3 camForward = glm::vec3(g_transform.to_mat4()[2]);
        glm::vec3 movementForwardVector = glm::normalize(glm::vec3(camForward.x, 0, camForward.z));

        // Speed factor
        float speedFactor = 1.0f;
        if (Input::KeyDown(GLFW_KEY_LEFT_SHIFT)) {
            speedFactor = 0.425f;
        }
        // WSAD
        glm::vec3 displacement = glm::vec3(0);
        if (Input::KeyDown(HELL_KEY_A)) {
            displacement -= camRight;
        }
        if (Input::KeyDown(HELL_KEY_D)) {
            displacement += camRight;
        }
        if (Input::KeyDown(HELL_KEY_W)) {
            displacement -= movementForwardVector;
        }
        if (Input::KeyDown(HELL_KEY_S)) {
            displacement += movementForwardVector;
        }
        displacement *= g_walkSpeed * deltaTime * speedFactor;
        g_transform.position += displacement;

        // View height
        float heightSpeed = 3.0f;
        if (Input::KeyDown(HELL_KEY_SPACE)) {
            g_transform.position.y += deltaTime * heightSpeed * speedFactor;
        }
        if (Input::KeyDown(GLFW_KEY_LEFT_CONTROL)) {
            g_transform.position.y -= deltaTime * heightSpeed * speedFactor;
        }
    }

    glm::mat4 GetProjectionMatrix() {
        int width, height;
        glfwGetWindowSize(g_window, &width, &height);
        return glm::perspective(1.0f, float(width) / float(height), NEAR_PLANE, FAR_PLANE);
    }

    glm::mat4 GetViewMatrix() {
        return glm::inverse(g_transform.to_mat4());
    }

    glm::mat4 GetInverseViewMatrix() {
        return g_transform.to_mat4();
    }

    glm::vec3 GetViewRotation() {
        return g_transform.rotation;
    }

    glm::vec3 GetViewPos() {
        return g_transform.position;
    }

    Transform GetTransform() {
        return g_transform;
    }

    glm::vec3 GetForward() {
       return -glm::vec3(GetViewMatrix()[0][2], GetViewMatrix()[1][2], GetViewMatrix()[2][2]);
    }

    glm::vec3 GetRight() {
        return glm::vec3(GetInverseViewMatrix()[0]);
    }
    glm::vec3 GetUp() {
        return glm::vec3(GetInverseViewMatrix()[1]);
    }
}

