#pragma once

#include "vulkan/vulkan.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unordered_map>
#include <vector>

namespace edl {

typedef uint32_t BufferIndex;

class UniformBufferObject;

class StagingBuffer {
public:
    StagingBuffer();
    virtual ~StagingBuffer();

    void create(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const VkDeviceSize& memorySize, const uint32_t& queueFamilyIndex);
    void destroy();

    void copyBufferToImage(const void* src, const VkImage& dst, const uint32_t& width, const uint32_t& height, const VkDeviceSize& size);
    void copyBufferToBuffer(const void* src, const VkBuffer& dst, const VkDeviceSize& dstOffset, const VkDeviceSize& size);
    void copyBufferToUBO(const void* src, const UniformBufferObject& dst, const uint32_t& index);

    void submit(const VkQueue& queue, const VkFence& fence = nullptr);
    void submit2(const VkQueue& queue, const VkFence& fence = nullptr);


private:
    VkDevice vulkan_device;

    VkDeviceSize size;
    VkBuffer buffer;
    VkDeviceMemory memory;

    VkDeviceSize current_offset;

    struct BufferCopyCommand {
        VkBuffer dst;
        VkBufferCopy buffer_copy;
    };

    struct ImageCopyCommand {
        VkImage dst;
        VkBufferImageCopy buffer_image_copy;
    };

    std::vector<BufferCopyCommand> buffer_copy_commands;
    std::vector<ImageCopyCommand> image_copy_commands;

    void* map;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
};

}