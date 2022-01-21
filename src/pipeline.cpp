#include "pipeline.h"

#include "edl/vk/vulkan.h"

namespace edl {

void Pipeline::init(rapidjson::GenericObject<false, rapidjson::Value>& pipeline_obj, const std::vector<std::string>& shaders, std::unordered_map<std::string, ShaderHandle>& shader_handles, std::unordered_map<std::string, ShaderReflection>& reflections, global_info* info) {

	//GENERATE FRAMEBUFFERS
	//TODO: Needs to be different
	{
		clearValues.resize(2);
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		imageviews.resize(info->num_back_buffers);
		framebuffers.resize(info->num_back_buffers);
		renderpassInfos.resize(info->num_back_buffers);

		for (uint32_t i = 0; i < info->num_back_buffers; ++i) {
			VkImageViewCreateInfo imageViewCreateInfo;
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.flags = 0;
			imageViewCreateInfo.pNext = NULL;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = info->swapchain_format;
			imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
			imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			imageViewCreateInfo.image = info->swapchain_images[i];
			vk::checkResult(vkCreateImageView(info->vulkan_device, &imageViewCreateInfo, NULL, &imageviews[i]), "Imageview Creation!");
		}
	}

	// Create Rendering info instead of stinky renderpass
	{
		attachments.resize(3);

		attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		attachments[0].pNext = nullptr;
		attachments[0].imageView = imageviews[0];// info-> //TODO: FIX
		attachments[0].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		attachments[0].resolveMode = VK_RESOLVE_MODE_NONE;
		attachments[0].resolveImageView = VK_NULL_HANDLE;
		attachments[0].resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };

		formats.push_back(info->swapchain_format); // Determine Formats should be determinable from the shader reflection

		attachments[1].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		attachments[1].pNext = nullptr;
		attachments[1].imageView = imageviews[1];// info-> //TODO: FIX
		attachments[1].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		attachments[1].resolveMode = VK_RESOLVE_MODE_NONE;
		attachments[1].resolveImageView = VK_NULL_HANDLE;
		attachments[1].resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };

		formats.push_back(info->swapchain_format); // Determine Formats should be determinable from the shader reflection

		attachments[2].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		attachments[2].pNext = nullptr;
		attachments[2].imageView = info->depth_handle.imageview;// info-> //TODO: FIX
		attachments[2].imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
		attachments[2].resolveMode = VK_RESOLVE_MODE_NONE;
		attachments[2].resolveImageView = VK_NULL_HANDLE;
		attachments[2].resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[2].clearValue.depthStencil = { 1, 0 };

		formats.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT); // Determine
	}
	{
		renderingInfos.resize(info->num_back_buffers);

		renderingInfos[0].sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfos[0].pNext = nullptr;
		renderingInfos[0].flags = 0;
		renderingInfos[0].renderArea.offset = { 0, 0 };
		renderingInfos[0].renderArea.extent = { info->width, info->height };
		renderingInfos[0].layerCount = 1;
		renderingInfos[0].viewMask = 0;
		renderingInfos[0].colorAttachmentCount = 1;
		renderingInfos[0].pColorAttachments = &attachments[0];
		renderingInfos[0].pDepthAttachment = &attachments[2];
		renderingInfos[0].pStencilAttachment = &attachments[2];

		renderingInfos[1].sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfos[1].pNext = nullptr;
		renderingInfos[1].flags = 0;
		renderingInfos[1].renderArea.offset = { 0, 0 };
		renderingInfos[1].renderArea.extent = { info->width, info->height };
		renderingInfos[1].layerCount = 1;
		renderingInfos[1].viewMask = 0;
		renderingInfos[1].colorAttachmentCount = 1;
		renderingInfos[1].pColorAttachments = &attachments[1];
		renderingInfos[1].pDepthAttachment = &attachments[2];
		renderingInfos[1].pStencilAttachment = &attachments[2];
	}

	// Load Pipeline
	{
		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos(shaders.size());
		for (int i = 0; i < shaders.size(); ++i) {
			shaderStageCreateInfos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCreateInfos[i].pNext = nullptr;
			shaderStageCreateInfos[i].flags = NULL;
			shaderStageCreateInfos[i].stage = shader_handles[shaders[i]].stage;
			shaderStageCreateInfos[i].module = shader_handles[shaders[i]].shader;
			shaderStageCreateInfos[i].pName = "main"; // This should always be true
			shaderStageCreateInfos[i].pSpecializationInfo = NULL;
		}

		for (int i = 0; i < shaders.size(); ++i) {
			reflection = reflection + reflections[shaders[i]];
		}

		VkDescriptorSetLayout imageLayout;

		std::vector<VkDescriptorSetLayout> setLayouts;
		for (auto& ptr = reflection.uniforms.begin(); ptr != reflection.uniforms.end(); ++ptr) {
			if (ptr->first == 8) { //TODO: HACK!!!
				bindings[ptr->first].push_back({});
				VkDescriptorSetLayoutBinding& binding = bindings[ptr->first].back();
				binding.binding = 0;
				binding.descriptorCount = 10000;
				binding.pImmutableSamplers = nullptr;
				binding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

				//descriptorSetLayouts.push_back(vk::createBindlessImageDescriptorSetLayout(info->vulkan_device, 10000, 0));
				imageLayout = vk::createBindlessImageDescriptorSetLayout(info->vulkan_device, 10000, 0);
				continue;
			}
			
			for (int i = 0; i < reflection.uniforms[ptr->first].size(); ++i) {
				bindings[ptr->first].push_back({});
				VkDescriptorSetLayoutBinding& binding = bindings[ptr->first].back();
				binding.binding = reflection.uniforms[ptr->first][i].binding;
				binding.descriptorCount = reflection.uniforms[ptr->first][i].count;
				binding.pImmutableSamplers = nullptr;
				binding.stageFlags = reflection.uniforms[ptr->first][i].stage;
				

				//TODO: make this better
				if (reflection.uniforms[ptr->first][i].datatype == glsl_datatype::sampler2D) {
					binding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				}
				else if (reflection.uniforms[ptr->first][i].datatype == glsl_datatype::ubo) {
					binding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				}
				else if (reflection.uniforms[ptr->first][i].datatype == glsl_datatype::sampler) {
					binding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
				}
				else if (reflection.uniforms[ptr->first][i].datatype == glsl_datatype::texture2D) {
					binding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				}
				else if (reflection.uniforms[ptr->first][i].datatype == glsl_datatype::buffer) {
					binding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
				}
			}

			descriptorSetLayouts.push_back(vk::createDescriptorSetLayout(info->vulkan_device, bindings[ptr->first].size(), bindings[ptr->first].data()));
		}
		descriptorSetLayouts.push_back(imageLayout);

		pipelineLayout = vk::createPipelineLayout(info->vulkan_device, descriptorSetLayouts.size(), descriptorSetLayouts.data());

		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(reflection.attributes.size());
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(reflection.attributes.size());
		for (int i = 0; i < reflection.attributes.size(); ++i) {
			attributeDescriptions[i].binding = i; //TODO: add bindings support, I don't know what they are, but we need support for them (:
			attributeDescriptions[i].format = reflection.attributes[i].format;
			attributeDescriptions[i].location = reflection.attributes[i].location;
			attributeDescriptions[i].offset = 0;

			bindingDescriptions[i].binding = i;
			bindingDescriptions[i].stride = reflection.attributes[i].size;
			bindingDescriptions[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX; //Something needs to happen here
		}

		VkPipelineVertexInputStateCreateInfo inputInfo = {};
		inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		inputInfo.pNext = NULL;
		inputInfo.flags = 0;
		inputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
		inputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
		inputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
		inputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)info->width;
		viewport.height = (float)info->height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = info->width;
		scissor.extent.height = info->height;

		VkPipelineViewportStateCreateInfo viewportInfo;
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.pNext = NULL;
		viewportInfo.flags = 0;
		viewportInfo.viewportCount = 1;
		viewportInfo.pViewports = &viewport;
		viewportInfo.scissorCount = 1;
		viewportInfo.pScissors = &scissor;

		//TODO: Yeah, this is all going to need support
		VkPipelineRasterizationStateCreateInfo rasterInfo;
		rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterInfo.pNext = NULL;
		rasterInfo.flags = 0;
		rasterInfo.depthClampEnable = VK_FALSE; //Clamps fragments outside of near/far instead of discarding
		rasterInfo.rasterizerDiscardEnable = VK_FALSE; //From lunarg, "controls whether primitives are discarded immediately before the rasterization stage"
		rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterInfo.lineWidth = 1.0f;
		rasterInfo.cullMode = VK_CULL_MODE_NONE;//VK_CULL_MODE_BACK_BIT;
		rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		//rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterInfo.depthBiasEnable = VK_FALSE;
		rasterInfo.depthBiasConstantFactor = 0.0f;
		rasterInfo.depthBiasClamp = 0.0f;
		rasterInfo.depthBiasSlopeFactor = 0.0f;

		//TODO: support
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleInfo.pNext = NULL;
		multisampleInfo.flags = 0;
		multisampleInfo.sampleShadingEnable = VK_FALSE;
		multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleInfo.minSampleShading = 1.0f;
		multisampleInfo.pSampleMask = NULL;
		multisampleInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleInfo.alphaToOneEnable = VK_FALSE;

		//TODO: support
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
		colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachmentState.blendEnable = VK_FALSE;
		colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

		//TODO: support
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendInfo.pNext = NULL;
		colorBlendInfo.flags = 0;
		colorBlendInfo.logicOpEnable = VK_FALSE;
		colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendInfo.attachmentCount = 1;
		colorBlendInfo.pAttachments = &colorBlendAttachmentState;
		colorBlendInfo.blendConstants[0] = 0.0f;
		colorBlendInfo.blendConstants[1] = 0.0f;
		colorBlendInfo.blendConstants[2] = 0.0f;
		colorBlendInfo.blendConstants[3] = 0.0f;

		//TODO: support
		VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = {};
		depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateInfo.pNext = NULL;
		depthStencilStateInfo.flags = 0;
		depthStencilStateInfo.depthTestEnable = VK_TRUE;
		depthStencilStateInfo.depthWriteEnable = VK_TRUE;
		depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilStateInfo.minDepthBounds = 0.0f;
		depthStencilStateInfo.maxDepthBounds = 1.0f;
		depthStencilStateInfo.stencilTestEnable = VK_FALSE;

		//TODO: support
		VkPipelineInputAssemblyStateCreateInfo assemblyStateCreateInfo = {};
		assemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assemblyStateCreateInfo.pNext = nullptr;
		assemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
		assemblyStateCreateInfo.flags = NULL;
		assemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		//TODO: support
		//Dynamic state nonsense
		VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
		};

		//TODO: support
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;

		VkPipelineRenderingCreateInfoKHR infoKHR = {};
		infoKHR.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		infoKHR.pNext = nullptr;
		infoKHR.colorAttachmentCount = 1;
		infoKHR.pColorAttachmentFormats = &formats[0];
		infoKHR.depthAttachmentFormat = formats[2];
		infoKHR.stencilAttachmentFormat = formats[2];
		infoKHR.viewMask = 0;

		//TODO: support
		VkGraphicsPipelineCreateInfo pipelineInfo;
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = &infoKHR;
		pipelineInfo.flags = 0;
		pipelineInfo.stageCount = shaderStageCreateInfos.size();
		pipelineInfo.pStages = shaderStageCreateInfos.data();

		pipelineInfo.pVertexInputState = &inputInfo;
		pipelineInfo.pInputAssemblyState = &assemblyStateCreateInfo;
		pipelineInfo.pViewportState = &viewportInfo;
		pipelineInfo.pRasterizationState = &rasterInfo;
		pipelineInfo.pMultisampleState = &multisampleInfo;
		pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
		pipelineInfo.pColorBlendState = &colorBlendInfo;
		pipelineInfo.pDynamicState = &dynamicState;

		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = VK_NULL_HANDLE;
		pipelineInfo.subpass = 0;

		pipelineInfo.basePipelineHandle = NULL;
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.pTessellationState = NULL;

		vkCreateGraphicsPipelines(info->vulkan_device, NULL, 1, &pipelineInfo, NULL, &pipeline);
	}
	
	VkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(info->vulkan_device, "vkCmdBeginRenderingKHR");
}

void Pipeline::bind(VkCommandBuffer& cb, uint32_t imageIndex) {
	//vkCmdBeginRenderPass(cb, &renderpassInfos[imageIndex], VK_SUBPASS_CONTENTS_INLINE);

	VkCmdBeginRenderingKHR(cb, &renderingInfos[imageIndex]);
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

}