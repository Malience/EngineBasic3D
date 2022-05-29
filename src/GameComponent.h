#pragma once

#include "GameObject.h"
#include "ResourceHandles.h"
#include "edl/resource.h"

namespace edl {

class GameObject;

class GameComponent {
public:
    virtual void update(Toolchain& toolchain, float delta) = 0;

    GameObject* parent;
};

}