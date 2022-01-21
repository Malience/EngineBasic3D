#include "descriptor_manager.h"

#include "UniformBufferObject.h"
#include "image_table.h"
#include "Util.h"
#include "edl/io.h"
#include "edl/vk/vulkan.h"

#include "vulkan/vulkan.h"

#include <assert.h>

namespace edl {

DescriptorManager::DescriptorManager() {}
void DescriptorManager::init(const VkDevice& device) {
	vulkan_device = device;

	uniform_descriptor_pool = vk::createDescriptorPool(device, 10000, 10000, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
	image_descriptor_pool = vk::createDescriptorPool(device, 20000, 20000, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
}

DescriptorManager::~DescriptorManager() {
    
}

DescriptorHandle DescriptorManager::create(DescriptorSetLayout& layout) {
	DescriptorHandle handle = {};
	handle.layout = layout;

	//TODO: Make it not crash
	if (layout.type == PoolTypeBits::UNIFORM_BIT) {
		vk::allocateDescriptorSets(vulkan_device, uniform_descriptor_pool, 1, &layout.layout, &handle.descriptorSet);
	} 
	//TODO: Fix this
	else {// if (layout.type == PoolTypeBits::IMAGE_BIT) {
		vk::allocateDescriptorSets(vulkan_device, image_descriptor_pool, 1, &layout.layout, &handle.descriptorSet);
	}
	handle.vulkan_device = vulkan_device;
	return handle;
}

DescriptorSetLayout DescriptorManager::createLayout(std::vector<VkDescriptorSetLayoutBinding>& bindings) {
	DescriptorSetLayout layout;
	layout.layout = vk::createDescriptorSetLayout(vulkan_device, bindings.size(), bindings.data());
	layout.bindings = bindings;
	layout.type = PoolTypeBits::NONE;
	for (int i = 0; i < bindings.size(); ++i) {
		layout.type = (PoolTypeBits)((uint32_t)layout.type | (0x1 << bindings[i].descriptorType));
	}

	return layout;
}

void DescriptorHandle::bindBuffer(const VkBuffer& buffer, const VkDeviceSize& range, const VkDeviceSize& offset, const uint32_t binding) {
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = buffer;
	bufferInfo.range = range;
	bufferInfo.offset = offset;
	
	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = NULL;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	write.descriptorCount = 1;
	write.dstSet = descriptorSet;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.pImageInfo = nullptr;
	write.pBufferInfo = &bufferInfo;
	write.pTexelBufferView = NULL;

	vkUpdateDescriptorSets(vulkan_device, 1, &write, 0, NULL);
}

void DescriptorHandle::bindImage(const VkImageView& imageview, const VkSampler& sampler, const uint32_t binding) {
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageView = imageview;
	imageInfo.sampler = sampler;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = NULL;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.descriptorCount = 1;
	write.dstSet = descriptorSet;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.pImageInfo = &imageInfo;
	write.pBufferInfo = nullptr;
	write.pTexelBufferView = NULL;

	vkUpdateDescriptorSets(vulkan_device, 1, &write, 0, NULL);
}

void DescriptorHandle::bindImages(const std::vector <VkSampler>& samplers, const std::vector<VkImageView>& imageviews, const uint32_t binding) {
	assert(samplers.size() == imageviews.size());
	
	std::vector<VkDescriptorImageInfo> imageInfos(imageviews.size());
	for (int i = 0; i < imageviews.size(); ++i) {
		imageInfos[i].imageView = imageviews[i];
		imageInfos[i].sampler = samplers[i];
		imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = NULL;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.descriptorCount = imageviews.size();
	write.dstSet = descriptorSet;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.pImageInfo = imageInfos.data();
	write.pBufferInfo = nullptr;
	write.pTexelBufferView = NULL;

	vkUpdateDescriptorSets(vulkan_device, 1, &write, 0, NULL);
}

// Deprecated
void DescriptorHandle::bindImages(const VkSampler& sampler, const std::vector<VkImageView>& imageviews, const uint32_t samplerBinding, const uint32_t imageBinding) {
	VkDescriptorImageInfo samplerInfo = {};
	samplerInfo.imageView = nullptr;
	samplerInfo.sampler = sampler;
	samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	std::vector<VkDescriptorImageInfo> imageInfos(imageviews.size());
	for (int i = 0; i < imageviews.size(); ++i) {
		imageInfos[i].imageView = imageviews[i];
		imageInfos[i].sampler = nullptr;
		imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	VkWriteDescriptorSet write[2] = {};
	write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write[0].pNext = NULL;
	write[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	write[0].descriptorCount = 1;
	write[0].dstSet = descriptorSet;
	write[0].dstBinding = samplerBinding;
	write[0].dstArrayElement = 0;
	write[0].pImageInfo = &samplerInfo;
	write[0].pBufferInfo = nullptr;
	write[0].pTexelBufferView = NULL;

	write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write[1].pNext = NULL;
	write[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	write[1].descriptorCount = imageviews.size();
	write[1].dstSet = descriptorSet;
	write[1].dstBinding = imageBinding;
	write[1].dstArrayElement = 0;
	write[1].pImageInfo = imageInfos.data();
	write[1].pBufferInfo = nullptr;
	write[1].pTexelBufferView = NULL;

	vkUpdateDescriptorSets(vulkan_device, 2, write, 0, NULL);
}

}