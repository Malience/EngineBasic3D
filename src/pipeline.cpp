#include "pipeline.h"

#include "edl/vk/vulkan.h"

namespace edl {

Pipeline::Pipeline() : bindpoint(BindPoint::GRAPHICS), shaders(), pipeline(NULL) {}

void Pipeline::addShader(const std::string& shader) {
	shaders.push_back(shader);
}

void Pipeline::setBindPoint(BindPoint bp) {
	bindpoint = bp;
}
void Pipeline::setColorFormat(VkFormat format) {
	colorFormat = format;
}

void Pipeline::setDepthFormat(VkFormat format) {
	depthStencilFormat = format;
}

const VkPipelineBindPoint VK_BIND_POINTS[static_cast<uint32_t>(Pipeline::BindPoint::BIND_POINT_MAX)] = {
	VK_PIPELINE_BIND_POINT_GRAPHICS,
	VK_PIPELINE_BIND_POINT_COMPUTE,
	VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR
};

void Pipeline::bind(VkCommandBuffer& cb) {
	vkCmdBindPipeline(cb, VK_BIND_POINTS[static_cast<uint32_t>(bindpoint)], pipeline);
}

bool Pipeline::rebuild(Toolchain& toolchain) {
	ResourceSystem& system = toolchain.getTool<ResourceSystem>("system");
	vk::Instance& instance = toolchain.getTool<vk::Instance>("VulkanInstance");
	vk::Swapchain& swapchain = toolchain.getTool<vk::Swapchain>("VulkanSwapchain");

	// Load Pipeline
	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos(shaders.size());
	for (int i = 0; i < shaders.size(); ++i) {
		auto& shader = system.shader_handles[shaders[i]];

		shaderStageCreateInfos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCreateInfos[i].pNext = nullptr;
		shaderStageCreateInfos[i].flags = NULL;
		shaderStageCreateInfos[i].stage = shader.stage;
		shaderStageCreateInfos[i].module = shader.shader;
		shaderStageCreateInfos[i].pName = "main"; // This should always be true
		shaderStageCreateInfos[i].pSpecializationInfo = NULL;
	}

	//TODO: This might not work?
	VkPipelineVertexInputStateCreateInfo inputInfo = {};
	inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	inputInfo.pNext = NULL;
	inputInfo.flags = 0;
	inputInfo.vertexBindingDescriptionCount = 0;
	inputInfo.pVertexBindingDescriptions = nullptr;
	inputInfo.vertexAttributeDescriptionCount = 0;
	inputInfo.pVertexAttributeDescriptions = nullptr;

	// Blank viewport since viewport is dynamically supported
	VkPipelineViewportStateCreateInfo viewportInfo;
	viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.pNext = NULL;
	viewportInfo.flags = 0;
	viewportInfo.viewportCount = 1;
	viewportInfo.pViewports = {};
	viewportInfo.scissorCount = 1;
	viewportInfo.pScissors = {};

	// TODO: Support still needed for depthClampEnable, rasterizerDiscardEnable, polygonMode, deopthBias, and any EXT
	VkPipelineRasterizationStateCreateInfo rasterInfo;
	rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterInfo.pNext = nullptr;
	rasterInfo.flags = 0;
	rasterInfo.depthClampEnable = VK_FALSE; //Clamps fragments outside of near/far instead of discarding
	rasterInfo.rasterizerDiscardEnable = VK_FALSE; //From lunarg, "controls whether primitives are discarded immediately before the rasterization stage"
	rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterInfo.lineWidth = 0.0f;
	rasterInfo.cullMode = VK_CULL_MODE_NONE;
	rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
	depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateInfo.front = {};
    depthStencilStateInfo.back = {};
	depthStencilStateInfo.minDepthBounds = 0.0f;
	depthStencilStateInfo.maxDepthBounds = 1.0f;

	//TODO: support
	VkPipelineInputAssemblyStateCreateInfo assemblyStateCreateInfo = {};
	assemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assemblyStateCreateInfo.pNext = nullptr;
	assemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
	assemblyStateCreateInfo.flags = NULL;
	assemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Dynamic states currently supported
	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH,
		VK_DYNAMIC_STATE_FRONT_FACE,
		VK_DYNAMIC_STATE_CULL_MODE
	};

	// Setup Dynamic State
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = dynamicStates.size();
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineRenderingCreateInfoKHR infoKHR = {};
	infoKHR.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	infoKHR.pNext = nullptr;
	infoKHR.colorAttachmentCount = 1;
	infoKHR.pColorAttachmentFormats = &colorFormat;
	infoKHR.depthAttachmentFormat = depthStencilFormat;
	infoKHR.stencilAttachmentFormat = depthStencilFormat;
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

	pipelineInfo.layout = system.pipelineLayout;
	pipelineInfo.renderPass = VK_NULL_HANDLE;
	pipelineInfo.subpass = 0;

	pipelineInfo.basePipelineHandle = NULL;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.pTessellationState = NULL;

	vk::checkResult(vkCreateGraphicsPipelines(instance.device, NULL, 1, &pipelineInfo, NULL, &pipeline), "Graphics Pipeline Creation");

	return true;
}

}