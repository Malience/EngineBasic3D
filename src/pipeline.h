#pragma once

#include "ShaderReflection.h"
#include "GlobalInfo.h"
#include "ResourceSystem.h"
#include "edl/vk/vulkan.h"

#include "vulkan/vulkan.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include <unordered_map>
#include <string>
#include <vector>

namespace edl {

//TODO: blah blah blah
/*
static const std::unordered_map<glsl_datatype, VkDescriptorType> DESCRIPTOR_TYPE_MAP = {
    {glsl_datatype::vec2, VkDescriptorType}
};
*/

class Pipeline {
public:
    Pipeline() : renderpass(NULL), pipeline(NULL) {}
    void init(rapidjson::GenericObject<false, rapidjson::Value>& pipeline_obj, const std::vector<std::string>& shaders, std::unordered_map<std::string, ShaderHandle>& shader_handles, std::unordered_map<std::string, ShaderReflection>& reflections, global_info* info);
    void bind(VkCommandBuffer& cb, uint32_t imageIndex);

    std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> bindings;
    VkRenderPass renderpass;

    std::vector<VkRenderingInfoKHR> renderingInfos;
    std::vector<VkRenderingAttachmentInfoKHR> attachments;
    std::vector<VkRenderingAttachmentInfoKHR> attachmentsBuffer;
    std::vector<VkFormat> formats;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    std::vector<VkImageView> imageviews;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkRenderPassBeginInfo> renderpassInfos;

    std::vector<VkClearValue> clearValues;

    PFN_vkCmdBeginRenderingKHR VkCmdBeginRenderingKHR;

    ShaderReflection reflection;

private:
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

};

}