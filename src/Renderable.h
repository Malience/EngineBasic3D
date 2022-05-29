#pragma once

#include "GameComponent.h"
#include "ResourceHandles.h"
#include "edl/resource.h"

namespace edl {

class Renderable : public GameComponent {
public:
    virtual void update(Toolchain& toolchain, float delta);

    uint32_t mvpHandle;
    ResourceID model;
};

}