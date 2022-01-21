#include "StagingBuffer.h"
#include "UniformBufferObject.h"
#include "Util.h"

#include "edl/io.h"

#include "vulkan/vulkan.h"

namespace edl {

StagingBuffer::StagingBuffer() {
	vulkan_device = nullptr;

	buffer = nullptr;
	memory = nullptr;

	command_pool = nullptr;
	command_buffer = nullptr;

	current_offset = 0;
	size = 0;
}

StagingBuffer::~StagingBuffer() {

}

void transitionImage(VkCommandBuffer cb, VkImage image, const VkFormat format, const VkImageLayout oldLayout, const VkImageLayout newLayout) {
	VkImageMemoryBarrier memoryBarrier;
	memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	memoryBarrier.pNext = NULL;

	memoryBarrier.oldLayout = oldLayout;
	memoryBarrier.newLayout = newLayout;

	//Eventually this will matter
	memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	memoryBarrier.image = image;
	memoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	VkPipelineStageFlagBits srcMask, dstMask;
	//TODO: Fix this mess
	switch (oldLayout) {
	case VK_IMAGE_LAYOUT_UNDEFINED:
		memoryBarrier.srcAccessMask = 0;
		srcMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	}

	switch (newLayout) {
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		dstMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		//TODO: We could possible use the image in other pipeline stages
		dstMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		memoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dstMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;// VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		break;
	}

	vkCmdPipelineBarrier(cb, srcMask, dstMask, 0, 0, NULL, 0, NULL, 1, &memoryBarrier);
}

void StagingBuffer::create(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const VkDeviceSize& memorySize, const uint32_t& queueFamilyIndex) {
	vulkan_device = device;

	{ // Staging buffer setup
		VkBufferCreateInfo bufferInfo = {
			/*.sType =*/ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			/*.pNext =*/ NULL,
			/*.flags =*/ 0,
			/*.size =*/ memorySize,
			/*.usage =*/ VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			/*.sharingMode =*/ VK_SHARING_MODE_EXCLUSIVE,
			/*.queueFamilyIndexCount =*/ 0,
			/*.pQueueFamilyIndices =*/ NULL
		};

		vkCreateBuffer(vulkan_device, &bufferInfo, NULL, &buffer);

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(vulkan_device, buffer, &memoryRequirements);

		uint32_t memoryType = getMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		size = memoryRequirements.size;
		VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, memoryRequirements.size, memoryType };

		vkAllocateMemory(vulkan_device, &allocateInfo, NULL, &memory);
		vkBindBufferMemory(vulkan_device, buffer, memory, 0);

		//TODO: Check if this breaks
		vkMapMemory(vulkan_device, memory, 0, size, 0, &map);
	}
	{ // Command pool setup
		VkCommandPoolCreateInfo poolCreateInfo;
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreateInfo.pNext = nullptr;
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolCreateInfo.queueFamilyIndex = queueFamilyIndex;

		vkCreateCommandPool(vulkan_device, &poolCreateInfo, nullptr, &command_pool);

		VkCommandBufferAllocateInfo bufferAllocateInfo;
		bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferAllocateInfo.pNext = nullptr;
		bufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		bufferAllocateInfo.commandPool = command_pool;
		bufferAllocateInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(vulkan_device, &bufferAllocateInfo, &command_buffer);
	}
}

void StagingBuffer::destroy() {
	vkUnmapMemory(vulkan_device, memory);
	vkFreeMemory(vulkan_device, memory, nullptr);
	vkDestroyBuffer(vulkan_device, buffer, nullptr);
}

void StagingBuffer::copyBufferToImage(const void* src, const VkImage& dst, const uint32_t& width, const uint32_t& height, const VkDeviceSize& size) {
	size_t aligned_offset = align(current_offset, 0x4);

	//vkMapMemory(vulkan_device, memory, aligned_offset, size, 0, &map);
	memcpy(static_cast<char*>(map) + aligned_offset, src, size);
	//vkUnmapMemory(vulkan_device, memory);

	image_copy_commands.push_back({});
	ImageCopyCommand& command = image_copy_commands.back();

	command.dst = dst;
	
	command.buffer_image_copy.bufferOffset = aligned_offset;
	command.buffer_image_copy.bufferRowLength = 0;
	command.buffer_image_copy.bufferImageHeight = 0;

	command.buffer_image_copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	command.buffer_image_copy.imageOffset = { 0, 0, 0 };
	command.buffer_image_copy.imageExtent = { width, height, 1 };

	current_offset = aligned_offset + size;
}

void StagingBuffer::copyBufferToBuffer(const void* src, const VkBuffer& dst, const VkDeviceSize& dstOffset, const VkDeviceSize& size) {
	size_t aligned_offset = align(current_offset, 0x4);
	
	//TODO: Persistent mapping
	//void* map;
	//vkMapMemory(vulkan_device, memory, aligned_offset, size, 0, &map);
	memcpy(static_cast<char*>(map) + aligned_offset, src, size);
	//memcpy(map, src, size);
	//vkUnmapMemory(vulkan_device, memory);

	buffer_copy_commands.push_back({});
	BufferCopyCommand& command = buffer_copy_commands.back();

	command.dst = dst;
	command.buffer_copy.srcOffset = aligned_offset;
	command.buffer_copy.size = size;
	command.buffer_copy.dstOffset = dstOffset;

	current_offset = aligned_offset + size;
}

void StagingBuffer::copyBufferToUBO(const void* src, const UniformBufferObject& dst, const uint32_t& index) {
	copyBufferToBuffer(src, dst.uniform_buffer, index * dst.uniform_size, dst.uniform_size);
}

void StagingBuffer::submit(const VkQueue& queue, const VkFence& fence) {
	VkCommandBufferBeginInfo beginInfo;
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	vkResetCommandPool(vulkan_device, command_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
	vkBeginCommandBuffer(command_buffer, &beginInfo);

	for (uint32_t i = 0; i < image_copy_commands.size(); ++i) {
		transitionImage(command_buffer, image_copy_commands[i].dst, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vkCmdCopyBufferToImage(command_buffer, buffer, image_copy_commands[i].dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy_commands[i].buffer_image_copy);
		transitionImage(command_buffer, image_copy_commands[i].dst, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	for (uint32_t i = 0; i < buffer_copy_commands.size(); ++i) {
		vkCmdCopyBuffer(command_buffer, buffer, buffer_copy_commands[i].dst, 1, &buffer_copy_commands[i].buffer_copy);
	}

	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo sinfo = {};
	sinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	sinfo.pNext = NULL;
	sinfo.waitSemaphoreCount = 0;
	sinfo.pWaitSemaphores = nullptr;
	sinfo.pWaitDstStageMask = nullptr;
	sinfo.signalSemaphoreCount = 0;
	sinfo.pSignalSemaphores = nullptr;
	sinfo.commandBufferCount = 1;
	sinfo.pCommandBuffers = &command_buffer;

	vkQueueSubmit(queue, 1, &sinfo, fence);

	current_offset = 0;
	image_copy_commands.clear();
	buffer_copy_commands.clear();
}

void StagingBuffer::submit2(const VkQueue& queue, const VkFence& fence) {
	VkCommandBufferBeginInfo beginInfo;
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	vkResetCommandPool(vulkan_device, command_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
	vkBeginCommandBuffer(command_buffer, &beginInfo);
	/*
	for (uint32_t i = 0; i < image_copy_commands.size(); ++i) {
		transitionImage(command_buffer, image_copy_commands[i].dst, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vkCmdCopyBufferToImage(command_buffer, buffer, image_copy_commands[i].dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy_commands[i].buffer_image_copy);
		transitionImage(command_buffer, image_copy_commands[i].dst, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	*/
	for (uint32_t i = 0; i < buffer_copy_commands.size(); ++i) {
		vkCmdCopyBuffer(command_buffer, buffer, buffer_copy_commands[i].dst, 1, &buffer_copy_commands[i].buffer_copy);
	}

	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo sinfo = {};
	sinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	sinfo.pNext = NULL;
	sinfo.waitSemaphoreCount = 0;
	sinfo.pWaitSemaphores = nullptr;
	sinfo.pWaitDstStageMask = nullptr;
	sinfo.signalSemaphoreCount = 0;
	sinfo.pSignalSemaphores = nullptr;
	sinfo.commandBufferCount = 1;
	sinfo.pCommandBuffers = &command_buffer;

	vkQueueSubmit(queue, 1, &sinfo, fence);

	current_offset = 0;
	image_copy_commands.clear();
	buffer_copy_commands.clear();
}

}