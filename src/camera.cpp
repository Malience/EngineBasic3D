#include "camera.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "glm/vec3.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

const float PI = M_PI;
const float PI_2 = M_PI_2;

namespace edl {

Camera::Camera() {
    window = nullptr;

    view = glm::identity<glm::mat4>();
    forward = { 0.0f, 0.0f, 1.0f };
    right = { 1.0f, 0.0f, 0.0f };
    up = { 0.0f, 1.0f, 0.0f };
    pos = { 30.0f, 0.0f, 0.0f };

    locked = false;
    prevX = 0; prevY = 0;
    rotX = 0; rotY = 0;
}

void Camera::init(GLFWwindow* window) {
    this->window = window;
}

void Camera::setPos(float x, float y, float z) {
    pos = { x, y, z };

    view = glm::lookAt(pos, this->forward, this->up);
}

void Camera::setRot(float x, float y) {
    rotX = x; rotY = y;
    glm::quat orientation = glm::quat(glm::vec3(rotY, -rotX, 0.0f));
    this->forward = orientation * glm::vec3(0.0f, 0.0f, 1.0f);
    this->forward = glm::normalize(this->forward);
    this->right = glm::normalize(glm::cross(this->forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    this->up = glm::normalize(glm::cross(this->right, this->forward));
    view = glm::lookAt(pos, this->forward, this->up);
}

glm::vec3 Camera::getPos() {
    return pos;
}

void Camera::update(float delta) {
    const float motion = 30.0f * delta * (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 3.0f : 1.0f);

    float forward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    float back = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    float left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    float right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    forward *= motion; back *= motion; left *= motion; right *= motion;

    bool state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);

    const float xmod = 0.08f * delta;
    const float ymod = 0.08f * delta;
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (state) {
        if (state != locked) {
            prevX = x; prevY = y;
            x = 0; y = 0;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        glfwSetCursorPos(window, 0.0, 0.0);

        rotX += x * xmod;
        if (rotX > glm::radians(360.0f)) rotX -= glm::radians(360.0f);
        if (rotX < 0.0f) rotX += glm::radians(360.0f);

        rotY += y * ymod;
        if (rotY > glm::radians(89.0f)) rotY = glm::radians(89.0f);
        if (rotY < glm::radians(-89.0f)) rotY = glm::radians(-89.0f);
    }
    else {
        if (state != locked) {
            glfwSetCursorPos(window, prevX, prevY);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    locked = state;

    glm::mat4 temp = glm::rotate(glm::identity<glm::mat4>(), rotX, this->up);

    glm::quat orientation = glm::quat(glm::vec3(rotY, -rotX, 0.0f));

    this->forward = orientation * glm::vec3(0.0f, 0.0f, 1.0f);
    this->forward = glm::normalize(this->forward);
    this->right = glm::normalize(glm::cross(this->forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    this->up = glm::normalize(glm::cross(this->right, this->forward));

    pos += this->forward * (forward - back);
    pos += this->right * (right - left);

    view = glm::lookAt(pos, pos + this->forward, this->up); //Might need to use true up
}

}