#include "image_table.h"
#include "descriptor_manager.h"
#include "UniformBufferObject.h"
#include "StagingBuffer.h"
#include "Util.h"
#include "Model.h"
#include "ResourceSystem.h"
#include "pipeline.h"
#include "camera.h"
#include "SceneLoader.h"
#include "ShaderLoader.h"
#include "IMGLoader.h"
#include "JSONLoader.h"
#include "OBJLoader.h"

#include "edl/glfw_lib.h"
#include "edl/vk/vulkan.h"
#include <edl/shader.h>
#include "edl/io.h"
#include "edl/debug.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi

#include <iostream>
#include <stdio.h>
#include <vector>

uint32_t WINDOW_WIDTH = 1280;
uint32_t WINDOW_HEIGHT = 720;

std::string applicationName = "EngineBasic3D";
std::string engineName = "EngineBasic3D";

edl::vk::Instance instance;
VkCommandPool commandPool;
VkCommandBuffer commandBuffer;
edl::vk::Surface surface;
edl::vk::Swapchain swapchain;

VkCommandPool presentPool[2];
VkCommandBuffer presentBuffers[2];

VkFramebuffer* framebuffers;
VkImageView* imageviews;

VkSemaphore imageReady, renderingComplete;

edl::ResourceSystem resourceSystem;


const std::string SCENE_FILE = "./res/scene/scene.json";

void init(std::string applicationName);

int main() {
	init(applicationName);
	//runRenderer();
	//termRenderer();
}

void remakeCommandBuffer(VkCommandBuffer cb, uint32_t imageIndex);

glm::mat4 persp(float fovy, float aspect, float near, float far) {
	float tanhalf = tan(fovy / 2.0f);

	glm::mat4 scale = glm::identity<glm::mat4>();
	scale[0][0] = 1.0f / (aspect * tanhalf);
	scale[1][1] = 1.0f / -tanhalf;

	// The Standard depth mapping calculation
	//float c1 = 2 * far * near / (near - far);
	//float c2 = (far + near) / (far - near);

	// I don't think I fully understand why this works, I just kinda took the limit as they approached infinite (n=1.0f, f=2.0f as f->inf) and these were my answers 
	// Nearless depth buffer
	float c1 = -2.0f;
	float c2 = 1.0f;

	glm::mat4 depthMap = glm::identity<glm::mat4>();
	depthMap[2][2] = -c2;
	depthMap[3][2] = c1;
	depthMap[3][3] = 0;
	depthMap[2][3] = -1;

	glm::mat4 m = scale * depthMap;

	return m;
}

void init(std::string applicationName) {
#ifdef _DEBUG
	edl::log::setLevel(edl::log::Level::trace);
#else
	edl::log::setLevel(edl::log::Level::trace);
#endif

	edl::GLFW::createWindow(WINDOW_WIDTH, WINDOW_HEIGHT, applicationName);

	//TODO: Deal with GLFW
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = edl::GLFW::getRequiredExtensions(&glfwExtensionCount);

	std::vector<const char*> layers;
	layers.push_back("VK_LAYER_KHRONOS_validation");

	std::vector<const char*> extensions;
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	for (uint32_t i = 0; i < glfwExtensionCount; ++i) extensions.push_back(glfwExtensions[i]);

	//Initialize Vulkan
	instance.create(applicationName, engineName, layers, extensions);
	
	VkPhysicalDeviceDynamicRenderingFeaturesKHR f = {};
	f.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	VkPhysicalDeviceFeatures2 f2 = {};
	f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	f2.pNext = &f;
	vkGetPhysicalDeviceFeatures2(instance.physicalDevice, &f2);

	//Create some command pools (Work needs to be done to create multiple)
	commandPool = edl::vk::createCommandPool(instance.device, 0, 0);
	edl::vk::createCommandBuffers(instance.device, commandPool, 1, &commandBuffer);

	//Create Surface
	surface.create(instance.instance, instance.physicalDevice, WINDOW_WIDTH, WINDOW_HEIGHT, edl::GLFW::getWindowHandle());
	swapchain.create(instance.physicalDevice, instance.device, surface);

	resourceSystem.toolchain.add("VulkanInstance", &instance);
	resourceSystem.toolchain.add("VulkanSwapchain", &swapchain);
	edl::initJSONLoader(resourceSystem.toolchain);

	resourceSystem.registerLoadFunction("shader", edl::loadShader);
	//resourceSystem.registerLoadFunction("mesh", edl::loadMesh);
	resourceSystem.registerLoadFunction("obj", edl::loadOBJ);
	resourceSystem.registerLoadFunction("img", edl::loadIMG);
	resourceSystem.registerLoadFunction("json", edl::loadJSON);

	edl::initSceneLoader(resourceSystem.toolchain);

	resourceSystem.init();
	resourceSystem.queue = instance.queues[0];

	// MVP setup
	resourceSystem.proj = persp(glm::radians(45.0f), static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT), 1.0f, 2.0f);
	//resourceSystem.proj[1][1] *= -1;

	resourceSystem.camera.init(edl::GLFW::getWindow());
	//camera.setPos(-306.675659f, -4.20801783f, 153.468704f);
	resourceSystem.camera.setPos(0.0f, 5.0f, -10.0f);
	resourceSystem.camera.setRot(glm::radians(0.0f), glm::radians(30.0f));

	resourceSystem.depthHandle[0] = resourceSystem.imageTable.createDepth(WINDOW_WIDTH, WINDOW_HEIGHT);
	resourceSystem.depthHandle[1] = resourceSystem.imageTable.createDepth(WINDOW_WIDTH, WINDOW_HEIGHT);
	resourceSystem.width = WINDOW_WIDTH;
	resourceSystem.height = WINDOW_HEIGHT;

	resourceSystem.buildRenderInfo();
	//TEMP
	/*globalInfo.swapchain_images.resize(swapchain.imageCount);
	for (int i = 0; i < swapchain.imageCount; ++i) {
		globalInfo.swapchain_images[i] = swapchain.images[i];
	}*/

	//globalInfo.num_back_buffers = globalInfo.swapchain_images.size();
	
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	for (int i = 0; i < resourceSystem.swapchain.imageCount; ++i) {
		edl::vk::transitionImage(commandBuffer, resourceSystem.swapchain.images[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		edl::vk::transitionImage(commandBuffer, resourceSystem.depthHandle[i].image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = NULL;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(instance.queues[0], 1, &submitInfo, nullptr);
	vkQueueWaitIdle(instance.queues[0]);
	//commandBuffer.destroy();
	
	edl::Resource& res = resourceSystem.createResource("SceneFile", SCENE_FILE, "json", "Scene");
	resourceSystem.requestResourceLoad(res.id);
	
	//resourceSystem.update(0);

	//resourceSystem.updateModels(0);
	resourceSystem.stagingBuffer.submit(instance.queues[0]);
	vkQueueWaitIdle(instance.queues[0]);

	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = 0;

	vkCreateCommandPool(instance.device, &createInfo, nullptr, &presentPool[0]);
	vkCreateCommandPool(instance.device, &createInfo, nullptr, &presentPool[1]);

	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.pNext = nullptr;
	allocateInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandPool = presentPool[0];
	allocateInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(instance.device, &allocateInfo, &presentBuffers[0]);
	allocateInfo.commandPool = presentPool[1];
	vkAllocateCommandBuffers(instance.device, &allocateInfo, &presentBuffers[1]);

	VkSemaphoreCreateInfo seminfo = {};
	seminfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	seminfo.pNext = NULL;
	seminfo.flags = 0;

	vkCreateSemaphore(instance.device, &seminfo, nullptr, &imageReady);
	vkCreateSemaphore(instance.device, &seminfo, nullptr, &renderingComplete);

	resourceSystem.update(1);
	resourceSystem.rebuildPipelines();

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;
	fenceInfo.flags = 0;

	VkFence fence[2];
	vkCreateFence(instance.device, &fenceInfo, nullptr, &fence[0]);
	vkCreateFence(instance.device, &fenceInfo, nullptr, &fence[1]);

	//netengine.setup(resourceSystem.toolchain);

	//remakeCommandBuffer(presentBuffers[0], 0);
	//remakeCommandBuffer(presentBuffers[1], 1);

	double last = glfwGetTime();
	double check = last;
	double now, delta;
	double MS_PER_UPDATE = 1.0 / 60.0;
	double lag = 0.0;

	int frames = 0;
	while (!edl::GLFW::windowShouldClose()) {
		now = glfwGetTime();
		delta = now - last;
		last = now;
		lag += delta;

		edl::GLFW::pollEvents();

		while (lag >= MS_PER_UPDATE)
		{
			resourceSystem.camera.update(MS_PER_UPDATE);
			resourceSystem.update(MS_PER_UPDATE);
			lag -= MS_PER_UPDATE;
		}

		uint32_t imageIndex = 0;
		//Disable double buffering
		vkAcquireNextImageKHR(instance.device, swapchain.swapchain, UINT64_MAX, imageReady, VK_NULL_HANDLE, &imageIndex);
		//printf("Image Index: %d\n", imageIndex);
		
		vkWaitForFences(instance.device, 1, &fence[imageIndex], VK_TRUE, 10000000);
		vkDeviceWaitIdle(instance.device);
		vkResetFences(instance.device, 1, &fence[imageIndex]);
		remakeCommandBuffer(presentBuffers[imageIndex], imageIndex);

		//resourceSystem.updateModels(delta);
		resourceSystem.stagingBuffer.submit(instance.queues[0]);
		vkQueueWaitIdle(instance.queues[0]);


		VkPipelineStageFlags flags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = NULL;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageReady;
		submitInfo.pWaitDstStageMask = flags;
		submitInfo.signalSemaphoreCount = 0; // TODO: Fix this
		submitInfo.pSignalSemaphores = &renderingComplete;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &presentBuffers[imageIndex];

		vkQueueSubmit(instance.queues[0], 1, &submitInfo, fence[imageIndex]);
		vkQueueWaitIdle(instance.queues[0]);
		

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = NULL;
		presentInfo.waitSemaphoreCount = 0; //TODO : Fix this
		presentInfo.pWaitSemaphores = &renderingComplete;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain.swapchain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = NULL;

		if (vkQueuePresentKHR(instance.queues[0], &presentInfo) != VK_SUCCESS) {
			//TODO: FIX THIS
			printf("Failed to present!\n");
		}
		vkQueueWaitIdle(instance.queues[0]);

		++frames;
		if (now >= check + 1) {
			check = now;
			printf("Frames: %d\n", frames);
			frames = 0;
		}
	}
	
}

void remakeCommandBuffer(VkCommandBuffer cb, uint32_t imageIndex) {

	//vkResetCommandPool(instance.device, presentPool[imageIndex], 0);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = 0;
	beginInfo.pInheritanceInfo = 0;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkPipelineStageFlagBits srcMask, dstMask;
	VkImageMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	memoryBarrier.pNext = NULL;
	//Eventually this will matter
	memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	vkBeginCommandBuffer(cb, &beginInfo);

	memoryBarrier.image = resourceSystem.swapchain.images[imageIndex];
	memoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	memoryBarrier.srcAccessMask = 0;
	memoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	memoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	srcMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	dstMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	vkCmdPipelineBarrier(cb, srcMask, dstMask, 0, 0, NULL, 0, NULL, 1, &memoryBarrier);

	memoryBarrier.image = resourceSystem.depthHandle[imageIndex].image;
	memoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };
	memoryBarrier.srcAccessMask = 0;
	memoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	memoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	srcMask = (VkPipelineStageFlagBits)(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
	dstMask = (VkPipelineStageFlagBits)(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
	vkCmdPipelineBarrier(cb, srcMask, dstMask, 0, 0, NULL, 0, NULL, 1, &memoryBarrier);

	resourceSystem.draw(cb, imageIndex);

	memoryBarrier.image = resourceSystem.swapchain.images[imageIndex];
	memoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	memoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	memoryBarrier.dstAccessMask = 0;
	memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	memoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	srcMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dstMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	vkCmdPipelineBarrier(cb, srcMask, dstMask, 0, 0, NULL, 0, NULL, 1, &memoryBarrier);

	vkEndCommandBuffer(cb);
}