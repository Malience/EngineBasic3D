#pragma once

#include "vulkan/vulkan.h"

#include <unordered_map>
#include <vector>

namespace edl {

struct Attribute {
    VkDeviceSize offset;
    size_t nameHash;
};

//struct Descriptor { //TEMP: Needs to be set up in a way so that they get
//
//};

struct SubMesh {
    uint32_t indexStart;
    uint32_t indexCount;
    VkDescriptorSet set;
};

struct Mesh2 {
    VkBuffer buffer;
    
    uint32_t indexCount;
    VkDeviceSize indexOffset;

    uint32_t attributeCount;
    Attribute* attributes;

    uint32_t submeshCount;
    SubMesh* submeshes;
};

// Returns the number of bytes allocated
size_t callocMesh(void* memory, uint32_t attributeCount, uint32_t submeshCount);

}