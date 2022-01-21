#include "IMGLoader.h"

#include "ResourceSystem.h"

#include "stb/stb_image.h"

namespace edl {

void loadIMG(res::Toolchain& toolchain, res::Resource& res) {
    edl::ResourceSystem& system = toolchain.getTool<edl::ResourceSystem>("system");

    edl::res::allocateResourceData(res, sizeof(edl::res::Image), *system.allocator);
    edl::res::Image& image = edl::res::getResourceData<edl::res::Image>(res);

    int width, height, channels;

    stbi_uc* data = stbi_load(res.path, &width, &height, &channels, 4);

    //std::cout << stbi_failure_reason() << std::endl;

    image.image = system.imageTable.create(width, height);

    image.materialIndex = system.bindlessImageDescriptor.createHandle();

    system.stagingBuffer.copyBufferToImage(data, image.image.image, width, height, width * height * 4 * sizeof(unsigned char));

    system.bindlessImageDescriptor.update(system.vulkan_device, image.materialIndex, image.image.imageview, image.image.sampler);

    system.textureMap.insert({res.name, image.materialIndex});

    stbi_image_free(data);
    res.status = edl::res::ResourceStatus::LOADED;
}

}