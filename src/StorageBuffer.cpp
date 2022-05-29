#include "StorageBuffer.h"

#include "edl/util.h"

namespace edl {

StorageBuffer createStorageBuffer(vk::Instance& instance, size_t size, VkBufferUsageFlags usage) {
    VkDevice device = instance.device;

    StorageBuffer buffer = {};
    buffer.descriptorPool = vk::createDescriptorPool(device, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
    buffer.descriptorSetLayout = vk::createDescriptorSetLayout(device, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1, 0, VK_SHADER_STAGE_ALL, 0);
    buffer.descriptorSet = vk::allocateDescriptorSets(device, buffer.descriptorPool, buffer.descriptorSetLayout);

    //TODO: GET RID
    buffer.elementSize = 1;
    buffer.elementMax = size;
    buffer.memorySize = edl::align(size, 0x100);

    buffer.buffer = vk::createBuffer(device, buffer.memorySize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | usage);

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, buffer.buffer, &req);

    buffer.memory = vk::allocateMemory(device, buffer.memorySize, instance.getMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

    vk::bindBufferMemory(device, buffer.buffer, buffer.memory, 0);

    VkBufferDeviceAddressInfo addressInfo = {};
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.pNext = nullptr;
    addressInfo.buffer = buffer.buffer;

    buffer.address = vkGetBufferDeviceAddress(device, &addressInfo);

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = buffer.memorySize;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = 0;
    write.descriptorCount = 1;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    write.dstSet = buffer.descriptorSet;
    write.dstBinding = 0;
    write.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    return buffer;
}

StorageBuffer createStorageBuffer(vk::Instance& instance, uint32_t elementSize, uint32_t elementMax, VkBufferUsageFlags usage) {
    VkDevice device = instance.device;

    StorageBuffer buffer = {};
    buffer.descriptorPool = vk::createDescriptorPool(device, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
    buffer.descriptorSetLayout = vk::createDescriptorSetLayout(device, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1, 0, VK_SHADER_STAGE_ALL, 0);
    buffer.descriptorSet = vk::allocateDescriptorSets(device, buffer.descriptorPool, buffer.descriptorSetLayout);

    buffer.elementSize = elementSize;
    buffer.elementMax = elementMax;
    buffer.memorySize = edl::align(elementSize * elementMax, 0x100);

    buffer.buffer = vk::createBuffer(device, buffer.memorySize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | usage);

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, buffer.buffer, &req);

    buffer.memory = vk::allocateMemory(device, buffer.memorySize, instance.getMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

    vk::bindBufferMemory(device, buffer.buffer, buffer.memory, 0);

    VkBufferDeviceAddressInfo addressInfo = {};
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.pNext = nullptr;
    addressInfo.buffer = buffer.buffer;

    buffer.address = vkGetBufferDeviceAddress(device, &addressInfo);

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = buffer.memorySize;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = 0;
    write.descriptorCount = 1;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    write.dstSet = buffer.descriptorSet;
    write.dstBinding = 0;
    write.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    return buffer;
}

uint32_t getStorageBufferIndex(StorageBuffer& storageBuffer, uint32_t count, uint32_t align) {
    uint32_t index = storageBuffer.currentCount;

    if (align > 0) {
        uint32_t t = index % align;
        index += t;
        count += t;
    }

    storageBuffer.currentCount += count;
    return index;
}

uint64_t getStorageBufferAddress(StorageBuffer& storageBuffer, uint32_t index) {
    return storageBuffer.address + storageBuffer.elementSize * index;
}

void updateStorageBuffer(StagingBuffer& stagingBuffer, const StorageBuffer& dst, uint32_t index, const void* src, uint32_t count) {
    stagingBuffer.copyBufferToBuffer(src, dst.buffer, index * dst.elementSize, count * dst.elementSize);
}

void bindStorageBuffer(VkCommandBuffer cb, StorageBuffer& storageBuffer, VkPipelineLayout pipelineLayout, uint32_t set) {
    uint32_t o = 0;
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, set, 1, &storageBuffer.descriptorSet, 1, &o);
}

}