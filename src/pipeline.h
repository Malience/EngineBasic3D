#pragma once

#include "ResourceSystem.h"
#include "edl/vk/vulkan.h"

#include "vulkan/vulkan.h"

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
    Pipeline();

    enum class BindPoint : uint32_t {
        GRAPHICS = 0,
        COMPUTE = 1,
        RAY_TRACING = 2,

        BIND_POINT_MAX = 3
    };
    void addShader(const std::string& shader);
    void setBindPoint(BindPoint bp);
    void setColorFormat(VkFormat format);
    void setDepthFormat(VkFormat format);

    void bind(VkCommandBuffer& cb);

    bool rebuild(Toolchain& toolchain);

private:
    BindPoint bindpoint;
    std::vector<std::string> shaders;

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

    //std::vector<VkFormat> formats;
    VkFormat colorFormat;
    VkFormat depthStencilFormat;

    VkPipeline pipeline;

};

}