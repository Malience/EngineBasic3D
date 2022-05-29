#include "Renderable.h"

#include "ResourceSystem.h"
#include "edl/resource.h"

namespace edl {

void Renderable::update(Toolchain& toolchain, float delta) {
    edl::ResourceSystem& system = toolchain.getTool<edl::ResourceSystem>("system");

    glm::mat4 mvp = system.proj * system.camera.view * parent->transform;
    updateStorageBuffer(system.stagingBuffer, system.transformBuffer, mvpHandle, &mvp, 1);
}

}