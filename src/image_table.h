#pragma once

#include "vulkan/vulkan.h"

#include <stdio.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string.h>

namespace edl {

class DescriptorManager;
class ImageTable;

class ImageHandle {
public:
    ImageHandle() {}

    uint32_t width, height;

    VkImage image;
    VkImageView imageview;
    VkSampler sampler;

private:
    ImageHandle(const uint32_t width, const uint32_t height, const VkImage& image, const VkImageView& imageview, const VkSampler& sampler);

friend ImageTable;
};

class DepthHandle {
public:
    DepthHandle() {}
    uint32_t width, height;

    VkImage image;
    VkImageView imageview;

private:
    DepthHandle(const uint32_t width, const uint32_t height, const VkImage& image, const VkImageView& imageview);

friend ImageTable;
};

class ImageTable {
public:
    ImageTable();
    virtual ~ImageTable();

    void init(const VkPhysicalDevice& physicalDevice, const VkDevice& device, VkDeviceSize imageMemorySize);
    void term();

    ImageHandle create(const uint32_t& width, const uint32_t& height);
    DepthHandle createDepth(const uint32_t& width, const uint32_t& height);

    std::vector<VkImage> images;
    std::vector<VkImageView> imageviews;
    std::vector<VkSampler> samplers;

private:
    VkDevice vulkan_device;

    VkDeviceMemory image_memory;
    VkDeviceSize image_memory_size;
    VkDeviceSize image_memory_offset;

    friend DescriptorManager;
};

}