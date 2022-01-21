#pragma once

#include "vulkan/vulkan.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unordered_map>
#include <vector>

namespace edl {

class UniformBufferObject;
class DescriptorManager;
class StagingBuffer;

class UBOHandle {
public:
    UBOHandle() {}
    UBOHandle(const UBOHandle& handle) : buffer(handle.buffer), size(handle.size), offset(handle.offset) {}
    UBOHandle(const VkBuffer& buffer, const VkDeviceSize& size, const VkDeviceSize& offset) : buffer(buffer), size(size), offset(offset) {}

    VkBuffer buffer;
    VkDeviceSize size;
    VkDeviceSize offset;

private:

    friend UniformBufferObject;
};

class UniformBufferObject {
public:
    UniformBufferObject();
    virtual ~UniformBufferObject();

    void create(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const uint32_t& numBufferObjects, const VkDeviceSize& uniformSize, const VkDeviceSize& alignment);
    void destroy();

    UBOHandle getHandle();

private:
    VkDevice vulkan_device;

    uint32_t next_handle = 0;
    uint32_t num_buffer_objects;
    VkDeviceSize total_size;
    VkDeviceSize alignment;
    VkDeviceSize uniform_size;

    VkBuffer uniform_buffer;
    VkDeviceMemory uniform_memory;

    friend DescriptorManager;
    friend StagingBuffer;
};

}