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

    const glm::vec3& getForward() const {
        return forward;
    }
    const glm::vec3& getPosition() const {
        return pos;
    }
    const glm::vec3& getUp() const {
        return up;
    }
    const glm::vec3& getRight() const {
        return right;
    }
    float getRotX() const {
        return rotX;
    }
    float getRotY() const {
        return rotY;
    }

    glm::vec3 getPos();

    glm::mat4 view;

    GLFWwindow* window;

private:
    

    glm::vec3 forward, right, up;
    glm::vec3 pos;

    bool locked;

    float prevX, prevY;
    float rotX, rotY;
};

}