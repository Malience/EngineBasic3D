#pragma once

#include "GameComponent.h"
#include "ResourceHandles.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include <string>
#include <vector>

namespace edl {

class GameComponent;

class GameObject {
public:
    GameObject();
    void update(res::Toolchain& toolchain, float delta);
    void calculateTransform();

    void addChild(GameObject& object) {
        children.push_back(&object);
    }

    std::string name;

    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
    glm::mat4 transform;

    GameObject* parent;
    std::vector<GameObject*> children;
    GameComponent* component;
};

}