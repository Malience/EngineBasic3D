#include "ShaderLoader.h"

#include "edl/util.h"
#include "edl/io.h"

namespace edl {

void loadShader(Toolchain& toolchain, Resource& res) {
    edl::ResourceSystem& system = toolchain.getTool<edl::ResourceSystem>("system");

    if (system.shader_handles.find(res.name) != system.shader_handles.end()) return; // If it is already loaded, don't
    ShaderHandle& shandle = system.shader_handles[res.name];

    //TODO: Make this actually do resource stuff
    //loadResourceFile(res, *system.allocator);
    //res::File file = res::loadFile(res.path.c_str(), *system.allocator);

    ShaderType type; //TODO: create shader type map
    if (res.subtype == "vert") {
        type = ShaderType::SHADER_TYPE_GLSL_VERTEX_SHADER;
        shandle.stage = VK_SHADER_STAGE_VERTEX_BIT;
    }
    else if (res.subtype == "frag") {
        type = ShaderType::SHADER_TYPE_GLSL_FRAGMENT_SHADER;
        shandle.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    else if (res.subtype == "mesh") {
        type = ShaderType::SHADER_TYPE_GLSL_MESH_SHADER;
        shandle.stage = VK_SHADER_STAGE_MESH_BIT_NV;
    }

    //TODO: Vectors are the devil
    std::vector<char> code(filesize(res.path));
    loadFile(res.path, code);
    //memcpy(code.data(), file.data, file.size);

    //TODO: Get the name of the file?
    std::vector<char> spv;
    compileShader(res.name, type, code, spv);

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = NULL;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.codeSize = spv.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(spv.data());

    vkCreateShaderModule(system.vulkan_device, &shaderModuleCreateInfo, NULL, &shandle.shader);

    res.status = ResourceStatus::LOADED;
}

}