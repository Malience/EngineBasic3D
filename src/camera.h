#pragma once

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

namespace edl {

class Camera {
public:
    Camera();

    void init(GLFWwindow* window);
    void update(float delta);

    void setPos(float x, float y, float z);
    void setRot(float x, float y);

    glm::vec3 getPos();

    glm::mat4 view;

private:
    GLFWwindow* window;

    glm::vec3 forward, right, up;
    glm::vec3 pos;

    bool locked;

    float prevX, prevY;
    float rotX, rotY;
};

}