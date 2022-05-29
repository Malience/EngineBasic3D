#pragma once

#include "GameComponent.h"
#include "ResourceHandles.h"

#include "edl/resource.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include <string>
#include <vector>

namespace edl {

class GameComponent;

class GameObject {
public:
    GameObject();

    void update(Toolchain& toolchain, float delta);
    void calculateTransform();

    void addChild(GameObject& object) {
        children.push_back(&object);
        object.parent = this;
    }

    void removeChild(GameObject& object) {
        children.erase(std::remove(children.begin(), children.end(), &object), children.end());
        object.parent = nullptr;
    }

    void addComponent(const std::string& name, GameComponent& component);

    bool hasComponent(const std::string& name) {
        return components.find(name) != components.end();
    }

    GameComponent& getComponent(const std::string& name) {
        return *components.at(name);
    }

    const std::string& getName() {
        return name;
    }
    
    GameObject* getParent() {
        return parent;
    }

    const std::vector<GameObject*>& getChildren() {
        return children;
    }

    void setEnabled(bool e) {
        enabled = e;
    }

    bool getEnabled() {
        return enabled;
    }

    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
    glm::mat4 transform;

    std::unordered_map<std::string, GameComponent*> components;

private:
    std::string name;

    bool enabled = true;

    GameObject* parent;
    std::vector<GameObject*> children;
    

    friend class ObjectRegistry;
};

class ObjectRegistry {
public:
    /// <summary>
    /// Creates a new object
    /// </summary>
    /// <param name="name">The unique name of the object</param>
    /// <returns>The new object</returns>
    GameObject& createObject(const std::string& name) {
        if (registry.find(name) != registry.end()) {
            std::cout << "ObjectRegistry -> Attempted to create object with duplicate name: " << name << std::endl;
            throw "ObjectRegistry -> Attempted to create object with duplicate name";
        }
        
        registry.insert({ name, {} });

        GameObject& object = registry.at(name);
        
        object.name = name;
        object.position = glm::vec3(0);
        object.rotation = glm::quat(0, 0, 0, 0);
        object.scale = glm::vec3(1);
        object.calculateTransform();

        root.addChild(object);
        return object;
    }

    GameObject& getObject(const std::string& name) {
        if (registry.find(name) == registry.end()) {
            std::cout << "ObjectRegistry -> No object found with name: " << name << std::endl;
            throw "ObjectRegistry -> No object found with name";
        }
        return registry.at(name);
    }

    void detachFromRoot(GameObject& object) {
        root.removeChild(object);
    }

    void attachToRoot(GameObject& object) {
        root.addChild(object);
    }

    void update(Toolchain& toolchain, float delta) {
        root.update(toolchain, delta);
    }

    bool hasObject(const std::string& name) {
        return registry.find(name) != registry.end();
    }

private:
    GameObject root;
    std::unordered_map<std::string, GameObject> registry;
};

}