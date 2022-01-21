#include "GameObject.h"

#include "ResourceSystem.h"

namespace edl {

GameObject::GameObject() {
    name = "";

    position = glm::vec3(0);
    rotation = glm::quat(0, 0, 0, 0);
    scale = glm::vec3(0);
    transform = glm::identity<glm::mat4>();

    parent = nullptr;
    children.reserve(1);
    component = nullptr;
}

void GameObject::update(res::Toolchain& toolchain, float delta) {
    if (component != nullptr) {
        component->update(toolchain, delta);
    }

    for (GameObject* child : children) {
        child->update(toolchain, delta);
    }
}

void GameObject::calculateTransform() {
    glm::mat4 trans, rot, s;

    trans = glm::translate(glm::mat4(1), position);
    rot = glm::mat4_cast(rotation);
    s = glm::scale(glm::mat4(1), scale);

    transform = trans * rot * s;
}

}
