#pragma once

#include "UniformBufferObject.h"

#include "vulkan/vulkan.h"

#include <stdio.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string.h>

namespace edl {

class DescriptorManager;
class UniformBufferObject;
class ImageHandle;
class ImageTable;

enum class PoolTypeBits : uint32_t {
    NONE = 0,
    UNIFORM_BIT = (0x1 << VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC),
    IMAGE_BIT = (0x1 << VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
};

//TODO: fix all of this
class DescriptorSetLayout {
public:
    DescriptorSetLayout() {}
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    VkDescriptorSetLayout layout;
    PoolTypeBits type;

};

class DescriptorHandle {
public:
    DescriptorHandle() {}

    void bindBuffer(const VkBuffer& buffer, const VkDeviceSize& range, const VkDeviceSize& offset, const uint32_t binding);
    void bindImage(const VkImageView& imageview, const VkSampler& sampler, const uint32_t binding);
    void bindImages(const std::vector <VkSampler>& samplers, const std::vector<VkImageView>& imageviews, const uint32_t binding);
    void bindImages(const VkSampler& sampler, const std::vector<VkImageView>& imageviews, const uint32_t samplerBinding, const uint32_t imageBinding);

    VkDevice vulkan_device;
    VkDescriptorSet descriptorSet;
    DescriptorSetLayout layout;

friend DescriptorManager;
};

class DescriptorManager {
public:
    DescriptorManager();
    virtual ~DescriptorManager();

    void init(const VkDevice& device);
    DescriptorHandle create(DescriptorSetLayout& layout);
    DescriptorSetLayout createLayout(std::vector<VkDescriptorSetLayoutBinding>& bindings);

    std::vector<VkDescriptorSet> uniform_descriptor_sets;
    std::vector<VkDescriptorSet> image_descriptor_sets;

private:
    uint32_t next_handle = 0;

    VkDevice vulkan_device;

    VkDescriptorPool uniform_descriptor_pool;
    VkDescriptorPool image_descriptor_pool;
};

}