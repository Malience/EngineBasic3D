#pragma once

#include "image_table.h"

#include "edl/vk/vulkan.h"

#include "vulkan/vulkan.h"

#include <vector>

namespace edl {

struct global_info {
    VkInstance vulkan_instance;
    VkDevice vulkan_device;
    uint32_t num_back_buffers;

    std::vector<VkImage> swapchain_images;
    DepthHandle depth_handle;

    VkFormat swapchain_format;

    uint32_t width;
    uint32_t height;
};

}