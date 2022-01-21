#include "ShaderLoader.h"

#include "Util.h"
#include "edl/io.h"

namespace edl {

void loadShader(res::Toolchain& toolchain, res::Resource& res) {
    edl::ResourceSystem& system = toolchain.getTool<edl::ResourceSystem>("system");

    if (system.shader_handles.find(res.name) != system.shader_handles.end()) return; // If it is already loaded, don't
    ShaderHandle& shandle = system.shader_handles[res.name];

    //TODO: Make this actually do resource stuff
    //loadResourceFile(res, *system.allocator);
    res::File file = res::loadFile(res.path, *system.allocator);

    res::ResourceType verttype = hashString("vert");
    res::ResourceType fragtype = hashString("frag");

    ShaderType type; //TODO: create shader type map
    if (res.subtype == verttype) {
        type = ShaderType::SHADER_TYPE_GLSL_VERTEX_SHADER;
        shandle.stage = VK_SHADER_STAGE_VERTEX_BIT;
    }
    else if (res.subtype == fragtype) {
        type = ShaderType::SHADER_TYPE_GLSL_FRAGMENT_SHADER;
        shandle.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    //TODO: Vectors are the devil
    std::vector<char> code(file.size);
    memcpy(code.data(), file.data, file.size);

    //TODO: Get the name of the file?
    shader_spv spv = system.shader_compiler.compileShader(res.name, type, code);

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = NULL;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.codeSize = spv.size;
    shaderModuleCreateInfo.pCode = spv.code.data();

    vkCreateShaderModule(system.vulkan_device, &shaderModuleCreateInfo, NULL, &shandle.shader);

    res.status = res::ResourceStatus::LOADED;
}

}