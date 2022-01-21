#include "image_table.h"

#include "Util.h"
#include "edl/io.h"

namespace edl {

ImageHandle::ImageHandle(const uint32_t width, const uint32_t height, const VkImage& image, const VkImageView& imageview, const VkSampler& sampler) : 
	width(width), height(height), image(image), imageview(imageview), sampler(sampler) {

}

DepthHandle::DepthHandle(const uint32_t width, const uint32_t height, const VkImage& image, const VkImageView& imageview) :
	width(width), height(height), image(image), imageview(imageview) {

}

ImageTable::ImageTable() {
	vulkan_device = nullptr;

	image_memory_size = 0;
	image_memory_offset = 0;
}

ImageTable::~ImageTable() {

}

void ImageTable::init(const VkPhysicalDevice& physicalDevice, const VkDevice& device, VkDeviceSize imageMemorySize) {
	vulkan_device = device;

	image_memory_size = imageMemorySize;

	{ // Image memory setup
		VkImageCreateInfo imageInfo;
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.pNext = NULL;
		imageInfo.flags = 0;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = 64;
		imageInfo.extent.height = 64;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.queueFamilyIndexCount = 0;
		imageInfo.pQueueFamilyIndices = NULL;

		VkImage tempImage;
		vkCreateImage(vulkan_device, &imageInfo, nullptr, &tempImage);

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(vulkan_device, tempImage, &memoryRequirements);

		VkMemoryAllocateInfo allocInfo;
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = NULL;
		allocInfo.memoryTypeIndex = getMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		allocInfo.allocationSize = image_memory_size;

		vkAllocateMemory(vulkan_device, &allocInfo, NULL, &image_memory);

		vkDestroyImage(vulkan_device, tempImage, nullptr);
	}
}

void ImageTable::term() {

}

ImageHandle ImageTable::create(const uint32_t& width, const uint32_t& height) {
	VkImageCreateInfo imageInfo;
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.pNext = NULL;
	imageInfo.flags = 0;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.pQueueFamilyIndices = NULL;

	images.push_back(NULL);
	VkImage& image = images.back();

	vkCreateImage(vulkan_device, &imageInfo, nullptr, &image);

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(vulkan_device, image, &memoryRequirements);
	
	image_memory_offset = align(image_memory_offset, memoryRequirements.alignment);

	vkBindImageMemory(vulkan_device, image, image_memory, image_memory_offset);
	image_memory_offset += memoryRequirements.size;

	imageviews.push_back(NULL);
	VkImageView& imageview = imageviews.back();

	VkImageViewCreateInfo imageviewInfo;
	imageviewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageviewInfo.pNext = nullptr;
	imageviewInfo.flags = 0;

	imageviewInfo.image = image;
	imageviewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageviewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;

	imageviewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	imageviewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };

	vkCreateImageView(vulkan_device, &imageviewInfo, NULL, &imageview);

	samplers.push_back(NULL);
	VkSampler& sampler = samplers.back();

	VkSamplerCreateInfo samplerInfo;
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.pNext = NULL;
	samplerInfo.flags = 0;

	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	vkCreateSampler(vulkan_device, &samplerInfo, NULL, &sampler);

	return ImageHandle(width, height, image, imageview, sampler);
}

DepthHandle ImageTable::createDepth(const uint32_t& width, const uint32_t& height) {
	VkImageCreateInfo imageInfo;
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.pNext = NULL;
	imageInfo.flags = 0;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; // | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.pQueueFamilyIndices = NULL;

	images.push_back(NULL);
	VkImage& image = images.back();

	vkCreateImage(vulkan_device, &imageInfo, nullptr, &image);

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(vulkan_device, image, &memoryRequirements);

	image_memory_offset = align(image_memory_offset, memoryRequirements.alignment);

	vkBindImageMemory(vulkan_device, image, image_memory, image_memory_offset);
	image_memory_offset += memoryRequirements.size;

	imageviews.push_back(NULL);
	VkImageView& imageview = imageviews.back();

	VkImageViewCreateInfo imageviewInfo;
	imageviewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageviewInfo.pNext = nullptr;
	imageviewInfo.flags = 0;

	imageviewInfo.image = image;
	imageviewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageviewInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;

	imageviewInfo.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
	imageviewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };

	vkCreateImageView(vulkan_device, &imageviewInfo, NULL, &imageview);

	return DepthHandle(width, height, image, imageview);
}

}