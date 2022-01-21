#pragma once

#include "GameComponent.h"
#include "ResourceHandles.h"

namespace edl {

class Renderable : public GameComponent {
public:
    virtual void update(res::Toolchain& toolchain, float delta);

    uint32_t mvpHandle;
    res::ResourceID model;
};

}