#pragma once

#include "edl/vk/vulkan.h"

#include "vulkan/vulkan.h"

#include <vector>

namespace edl {

typedef uint32_t BindlessImageHandle;

//Eperimental
class BindlessImageDescriptor {
public:
    void create(VkDevice device, uint32_t binding, uint32_t descriptorCount) {
        this->descriptorCount = descriptorCount;

        descriptorPool = vk::createDescriptorPool(device, 1, descriptorCount, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        descriptorSetLayout = vk::createBindlessImageDescriptorSetLayout(device, descriptorCount, binding);
        descriptorSet = vk::allocateBindlessDescriptorSet(device, descriptorPool, descriptorSetLayout, descriptorCount);
    }

    BindlessImageHandle createHandle() {
        return nextHandle++;
    }

    void update(VkDevice device, BindlessImageHandle handle, VkImageView imageview, VkSampler sampler) {
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.sampler = sampler;
        imageInfo.imageView = imageview;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; //Support

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = 0;
        write.descriptorCount = 1;
        write.dstArrayElement = handle;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.dstSet = descriptorSet;
        write.dstBinding = 0;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

//private:
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;

    uint32_t descriptorCount;

    BindlessImageHandle nextHandle;

    
};

}