#include "IMGLoader.h"

#include "ResourceSystem.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace edl {

void loadIMG(Toolchain& toolchain, Resource& res) {
    //std::cout << "-- Loading IMG: " << res.path << " START!" << std::endl;
    edl::ResourceSystem& system = toolchain.getTool<edl::ResourceSystem>("system");

    edl::allocateResourceData(res, sizeof(edl::res::Image), *system.allocator);
    edl::res::Image& image = edl::getResourceData<edl::res::Image>(res);

    int width, height, channels;

    //stbi_set_flip_vertically_on_load(1);
    //std::cout << "--- Loading File: " << res.path << " START!" << std::endl;
    stbi_uc* data = stbi_load(res.path.c_str(), &width, &height, &channels, 4);
    //std::cout << "--- Loading File: " << res.path << " END!" << std::endl;

    //std::cout << stbi_failure_reason() << std::endl;

    image.image = system.imageTable.create(width, height);

    image.materialIndex = system.bindlessImageDescriptor.createHandle();

    system.stagingBuffer.copyBufferToImage(data, image.image.image, width, height, width * height * 4 * sizeof(unsigned char));

    system.bindlessImageDescriptor.update(system.vulkan_device, image.materialIndex, image.image.imageview, image.image.sampler);

    system.textureMap.insert({res.name, image.materialIndex});

    stbi_image_free(data);
    res.status = edl::ResourceStatus::LOADED;

    //std::cout << "-- Loading IMG: " << res.path << " END!" << std::endl;
}

}