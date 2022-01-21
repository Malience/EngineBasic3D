#pragma once

#include "GameObject.h"
#include "ResourceHandles.h"

namespace edl {

class GameObject;

class GameComponent {
public:
    virtual void update(res::Toolchain& toolchain, float delta) = 0;

    GameObject* parent;
};

}