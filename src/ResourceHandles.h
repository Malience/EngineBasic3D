#pragma once

#include "descriptor_manager.h"
#include "glm/glm.hpp"
#include "vulkan/vulkan.h"
#include "edl/allocator.h"
#include "image_table.h"
#include "edl/resource.h"

#include <string>
#include <unordered_map>

namespace edl {
namespace res {

class Pipeline;
class ResourceSystem;

struct Image {
    ImageHandle image;
    //VkDescriptorSet set = 0; //PLEASE!!!!!!
    //UBOHandle ubo_handle;
    int32_t materialIndex;
    uint32_t size;
    void* data;
};

struct Batch {
    uint32_t indexStart;
    uint32_t indexCount;
    ResourceID material;
};

struct Mesh {
    uint32_t positionOffset;
    uint32_t normalOffset;
    uint32_t texCoord0Offset;
    uint32_t texCoord1Offset;
    uint32_t indexOffset;

    uint32_t batchCount;
    Batch* batches;
};

//TODO: Maybe set up a toolchain style system? Like you compile a toolchain similar to a VkDevice and toss that around to things?
class MeshBuilder {
public:
    void addBatch(Batch batch);

    Mesh build(Allocator& allocator);
    void clear();

private:
    std::vector<Batch> batches;
};

}
}
