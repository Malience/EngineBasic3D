#include "UniformBufferObject.h"

#include "Util.h"

#include "edl/io.h"
#include "edl/util.h"

#include "vulkan/vulkan.h"

namespace edl {

UniformBufferObject::UniformBufferObject() {
}

UniformBufferObject::~UniformBufferObject() {

}

void UniformBufferObject::create(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const uint32_t& numBufferObjects, const VkDeviceSize& uniformSize, const VkDeviceSize& alignment) {
	vulkan_device = device;
	num_buffer_objects = numBufferObjects;
	uniform_size = edl::align(uniformSize, alignment);

	total_size = edl::align(uniform_size * num_buffer_objects, alignment);

	this->alignment = alignment;

	{ // Uniform memory setup
	VkBufferCreateInfo bufferInfo = {
		/*.sType =*/ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		/*.pNext =*/ NULL,
		/*.flags =*/ 0,
		/*.size =*/ total_size,
		/*.usage =*/ VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		/*.sharingMode =*/ VK_SHARING_MODE_EXCLUSIVE,
		/*.queueFamilyIndexCount =*/ 0,
		/*.pQueueFamilyIndices =*/ NULL
	};

	vkCreateBuffer(vulkan_device, &bufferInfo, NULL, &uniform_buffer);

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(vulkan_device, uniform_buffer, &memoryRequirements);

	uint32_t memoryType = getMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, edl::align(memoryRequirements.size, alignment), memoryType };

	vkAllocateMemory(vulkan_device, &allocateInfo, NULL, &uniform_memory);
	vkBindBufferMemory(vulkan_device, uniform_buffer, uniform_memory, 0);
	}
}

UBOHandle UniformBufferObject::getHandle() { 
	//TODO: I don't even
	return UBOHandle(uniform_buffer, uniform_size, uniform_size * next_handle++);
}

void UniformBufferObject::destroy() {

}

}