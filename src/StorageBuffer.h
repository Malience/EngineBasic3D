#pragma once

#include "Util.h"
#include "StagingBuffer.h"
#include "edl/vk/vulkan.h"

#include "vulkan/vulkan.h"

#include <vector>

namespace edl {

struct StorageBuffer {
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    VkBuffer buffer;
    VkDeviceMemory memory;

    uint32_t elementSize;
    uint32_t elementMax;
    VkDeviceSize memorySize;

    uint32_t currentCount;
};

StorageBuffer createStorageBuffer(vk::Instance& instance, uint32_t elementSize, uint32_t elementMax, VkBufferUsageFlags usage = 0);
uint32_t getStorageBufferIndex(StorageBuffer& storageBuffer, uint32_t count = 1);
void updateStorageBuffer(StagingBuffer& stagingBuffer, const StorageBuffer& dst, uint32_t index, const void* src, uint32_t count);
void bindStorageBuffer(VkCommandBuffer cb, StorageBuffer& storageBuffer, VkPipelineLayout pipelineLayout, uint32_t set);

}