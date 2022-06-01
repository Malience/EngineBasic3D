#include "ResourceSystem.h"

#include "pipeline.h"
#include "Util.h"
#include "ShaderLoader.h"
#include "bindless_types.h"
#include "edl/util.h"
#include "edl/debug.h"
#include "edl/io.h"
#include "edl/resource.h"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace edl {

ResourceSystem::ResourceSystem() {
    allocator = new DefaultAllocator();
}

ResourceSystem::~ResourceSystem() {

}

void ResourceSystem::init() {
    instance = toolchain.getTool<vk::Instance>("VulkanInstance");
    vulkan_physical_device = instance.physicalDevice;
    vulkan_device = instance.device;

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(vulkan_physical_device, &props);

    stagingBuffer.create(vulkan_physical_device, vulkan_device, (size_t) 1 * 1028 * 1028 * 1028, 0);
    imageTable.init(vulkan_physical_device, vulkan_device, (size_t) 1920 * 1080 * 4 * 20);
    uniformBufferObject.create(vulkan_physical_device, vulkan_device, 20000, sizeof(float) * 16, props.limits.minUniformBufferOffsetAlignment);
    textureIndexObject.create(vulkan_physical_device, vulkan_device, 20000, sizeof(DrawData), props.limits.minUniformBufferOffsetAlignment);
    descriptorManager.init(vulkan_device);

    uint32_t objectMax = 30000;
    uint32_t triangleMax = 64 * 1024 * 1024;
    uint32_t materialMax = 10000;

    bindlessImageDescriptor.create(instance.device, 0, materialMax);

    transformBuffer = createStorageBuffer(instance, sizeof(glm::mat4), objectMax * 10, 0);
    materialBuffer = createStorageBuffer(instance, sizeof(Material), 10000, 0);
    materialSetBuffer = createStorageBuffer(instance, sizeof(MaterialSet), 10, 0);
    drawDataBuffer = createStorageBuffer(instance, sizeof(DrawCommand), objectMax * 10, 0);

    geometryBuffer = createStorageBuffer(instance, (size_t) 2 * 1024 * 1024 * 1024, 0);

    sceneDataBuffer = createStorageBuffer(instance, sizeof(SceneData), 1);
    lightBuffer = createStorageBuffer(instance, sizeof(Light), 10);

    if (!instance.MESH_SHADING) {
        indexBuffer = createStorageBuffer(instance, sizeof(uint16_t) * 20 * 3, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        std::vector<uint16_t> indices(20 * 3);
        for (uint16_t i = 0; i < (20 * 3); i++) {
            indices[i] = i;
        }
        updateStorageBuffer(stagingBuffer, indexBuffer, 0, indices.data(), sizeof(uint16_t) * 20 * 3);

        indirectBuffer = createStorageBuffer(instance, sizeof(VkDrawIndexedIndirectCommand), objectMax, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    }

    std::vector<VkDescriptorSetLayout> setLayouts;
    setLayouts.push_back(bindlessImageDescriptor.descriptorSetLayout);
    setLayouts.push_back(sceneDataBuffer.descriptorSetLayout);
    setLayouts.push_back(drawDataBuffer.descriptorSetLayout);
    setLayouts.push_back(geometryBuffer.descriptorSetLayout);
    setLayouts.push_back(transformBuffer.descriptorSetLayout);
    setLayouts.push_back(materialBuffer.descriptorSetLayout);
    setLayouts.push_back(lightBuffer.descriptorSetLayout);

    pipelineLayout = vk::createPipelineLayout(instance.device, setLayouts.size(), setLayouts.data());

    toolchain.add("system", this);
    toolchain.add("ObjectRegistry", &objectRegistry);
    toolchain.add("DirLight", &dirLight);
    toolchain.add("Camera", &camera);

    //TODO: If something is messing up, it's probably this!
    renderables.reserve(1000);

    directories.push_back("");
}

void ResourceSystem::buildRenderInfo() {
    swapchain = toolchain.getTool<vk::Swapchain>("VulkanSwapchain");
    imageviews.resize(swapchain.imageCount);
    for (uint32_t i = 0; i < swapchain.imageCount; i++) {
        VkImageViewCreateInfo imageviewcreateinfo;
        imageviewcreateinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageviewcreateinfo.flags = 0;
        imageviewcreateinfo.pNext = nullptr;
        imageviewcreateinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageviewcreateinfo.format = swapchain.format;
        imageviewcreateinfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        imageviewcreateinfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        imageviewcreateinfo.image = swapchain.images[i];
        vk::checkResult(vkCreateImageView(vulkan_device, &imageviewcreateinfo, nullptr, &imageviews[i]), "imageview creation!");
    }

    attachments.resize(4);

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

    //formats.push_back(info->swapchain_format); // Determine Formats should be determinable from the shader reflection

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

    //formats.push_back(info->swapchain_format); // Determine Formats should be determinable from the shader reflection

    attachments[2].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    attachments[2].pNext = nullptr;
    attachments[2].imageView = depthHandle[0].imageview;// info-> //TODO: FIX
    attachments[2].imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
    attachments[2].resolveMode = VK_RESOLVE_MODE_NONE;
    attachments[2].resolveImageView = VK_NULL_HANDLE;
    attachments[2].resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[2].clearValue.depthStencil = { 100.0f, 0 };
    

    attachments[3].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    attachments[3].pNext = nullptr;
    attachments[3].imageView = depthHandle[1].imageview;// info-> //TODO: FIX
    attachments[3].imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
    attachments[3].resolveMode = VK_RESOLVE_MODE_NONE;
    attachments[3].resolveImageView = VK_NULL_HANDLE;
    attachments[3].resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[3].clearValue.depthStencil = { 100.0f, 0 };

    //formats.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT); // Determine

    renderingInfos.resize(swapchain.imageCount);

    renderingInfos[0].sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfos[0].pNext = nullptr;
    renderingInfos[0].flags = 0;
    renderingInfos[0].renderArea.offset = { 0, 0 };
    renderingInfos[0].renderArea.extent = { width, height };
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
    renderingInfos[1].renderArea.extent = { width, height };
    renderingInfos[1].layerCount = 1;
    renderingInfos[1].viewMask = 0;
    renderingInfos[1].colorAttachmentCount = 1;
    renderingInfos[1].pColorAttachments = &attachments[1];
    renderingInfos[1].pDepthAttachment = &attachments[3];
    renderingInfos[1].pStencilAttachment = &attachments[3];
}

void ResourceSystem::draw(VkCommandBuffer& cb, uint32_t imageIndex) {
    std::string currentPipeline = "";
    
    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = width;
    scissor.extent.height = height;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)width;
    viewport.height = (float)height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 10.0f;

    if (renderables.size() == 0) return;

    Pipeline& pipeline = instance.MESH_SHADING ? pipelines["Shader3D"] : pipelines["Shader3DVertex"];
    VkRenderingInfo& renderingInfo = renderingInfos[imageIndex];
    vkCmdBeginRendering(cb, &renderingInfo);

    pipeline.bind(cb);

    // Set Dynamic State
    vkCmdSetViewport(cb, 0, 1, &viewport);
    vkCmdSetScissor(cb, 0, 1, &scissor);
    vkCmdSetCullMode(cb, VK_CULL_MODE_BACK_BIT);
    vkCmdSetFrontFace(cb, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    
    // Bind textures array
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &bindlessImageDescriptor.descriptorSet, 0, nullptr);

    // TODO: Get rid of this
    //vkCmdBindIndexBuffer(cb, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

    // Push buffer addresses
    vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_ALL, 0, 8, &sceneDataBuffer.address);
    vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_ALL, 8, 8, &drawDataBuffer.address);
    vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_ALL, 16, 8, &lightBuffer.address);

    uint32_t drawID = 0;

    //TODO: Update lights

    for (int i = 0; i < renderables.size(); ++i) {

        if (renderables[i].parent == nullptr || !renderables[i].parent->getEnabled()) continue;

        Resource& res = resMap.at(renderables[i].model);
        Model& model = edl::getResourceData<Model>(res);

        if (resMap.find(model.mesh) == resMap.end()) {
            std::cout << "Mesh not found for model: " << res.name << std::endl;
            continue;
        }

        if (resMap.find(model.material) == resMap.end()) {
            std::cout << "Material not found for model: " << res.name << std::endl;
            continue;
        }

        Mesh& mesh = edl::getResourceData<Mesh>(resMap.at(model.mesh));
        PBRMaterial& material = edl::getResourceData<PBRMaterial>(resMap.at(model.material));

        Material mat;// = material;
        mat.tint = material.tint;
        mat.metallic = material.metallic;
        mat.roughness = material.roughness;
        mat.ao = material.ao;

        if (material.albedoTexture > 0) {
            Resource& ires = resMap.at(material.albedoTexture);
            res::Image& image = edl::getResourceData<res::Image>(ires);

            mat.albedoTexture = image.materialIndex;
        }
        else {
            mat.albedoTexture = -1;
        }

        if (material.normalTexture > 0) {
            Resource& ires = resMap.at(material.normalTexture);
            res::Image& image = edl::getResourceData<res::Image>(ires);

            mat.normalTexture = image.materialIndex;
        }
        else {
            mat.normalTexture = -1;
        }

        edl::updateStorageBuffer(stagingBuffer, materialBuffer, material.materialIndex, &mat, 1);

        MaterialSet set = {};
        set.materials[0] = edl::getStorageBufferAddress(materialBuffer, material.materialIndex);

        edl::updateStorageBuffer(stagingBuffer, materialSetBuffer, 0, &set, 1);

        for (uint32_t j = 0; j < mesh.indexCount; j++) {
            DrawCommand drawCommand = {};
            drawCommand.mesh = mesh.positionOffset;
            drawCommand.meshlet = j;
            drawCommand.materials = edl::getStorageBufferAddress(materialSetBuffer, 0);
            drawCommand.mvp = edl::getStorageBufferAddress(transformBuffer, renderables[i].mvpHandle);

            edl::updateStorageBuffer(stagingBuffer, drawDataBuffer, drawID, &drawCommand, 1);

            if (!instance.MESH_SHADING) {
                VkDrawIndexedIndirectCommand indirect = {};
                indirect.firstIndex = 0;
                indirect.firstInstance = 0;
                indirect.instanceCount = 1;
                indirect.indexCount = (j == mesh.indexCount - 1 ? mesh.indexOffset % 20 : 20) * 3;
                indirect.vertexOffset = 0;

                edl::updateStorageBuffer(stagingBuffer, indirectBuffer, drawID, &indirect, 1);
            }

            drawID++;
        }
    }
    
    if (instance.MESH_SHADING) {
        PFN_vkCmdDrawMeshTasksNV vkCmdDrawMeshTasksNV = (PFN_vkCmdDrawMeshTasksNV)vkGetInstanceProcAddr(instance.instance, "vkCmdDrawMeshTasksNV");
        vkCmdDrawMeshTasksNV(cb, drawID, 0);
    }
    else {
        vkCmdBindIndexBuffer(cb, indexBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT16);
        //vkCmdDrawIndexed(cb, 20, drawID, 0, 0, 0);
        vkCmdDrawIndexedIndirect(cb, indirectBuffer.buffer, 0, drawID, sizeof(VkDrawIndexedIndirectCommand));
    }

    vkCmdEndRendering(cb);
}

void ResourceSystem::registerLoadFunction(const std::string& type, LoadFunction function) {
    if (loadFunctionRegistry.find(type) != loadFunctionRegistry.end()) {
        std::cout << "Loader already registered for type: " << type << std::endl;
    }

    loadFunctionRegistry.insert({ type, function });
}

void ResourceSystem::update(float delta) {
    //fileLoader.loadChunks(100);
    while (queuebot != queuetop) {
        ResourceID id = loadQueue[queuebot];

        Resource& res = resMap.at(id);
        //TODO: Check requirements loaded
        if (res.status == ResourceStatus::UNLOADED) {
            LoadFunction loadFunction = loadFunctionRegistry.at(res.type);

            loadFunction(toolchain, res);
        }

        loadQueue[queuebot] = 0;
        queuebot++;
        if (queuebot == MAX_QUEUE) queuebot = 0;
    }

    //TODO: Lol, this is a hack and a half
    //if (glfwGetKey(camera.window, GLFW_KEY_F5) == GLFW_PRESS) {
    //    refreshScene("scene.json");
    //}

    glm::vec3 campos = camera.getPos();
    SceneData scene = {};
    scene.activeLights = 0;
    scene.cameraPosition = glm::vec4(campos, 1.0f);
    scene.directionalLightPower = dirLight.directionalLightPower;
    scene.lightDir = dirLight.lightDir;
    scene.lightColor = dirLight.lightColor;

    edl::updateStorageBuffer(stagingBuffer, sceneDataBuffer, 0, &scene, 1);
    //fileLoader.cleanup();

    objectRegistry.update(toolchain, delta);
}

//Maybe multiple create functions, one for named resources, unnamed resources, and filenamed resources
Resource& ResourceSystem::createResource(const std::string& name, const std::string& filename, const std::string& type, const std::string& subtype) {
    uint64_t nameHash = hash(name);
    uint64_t filenameHash = hash(filename);

    ResourceID id = nameHash;//filenameHash == 0 ? nameHash : filenameHash;
    if (resMap.find(id) != resMap.end()) { //TODO: IDing needs to be fixed, need support for multiple resources loaded from the same file
        //printf("Resource already has name: %s\n", name.c_str());
        return resMap.at(id);
    }

    Resource& res = resMap[id];
    res.id = id;

    res.status = ResourceStatus::UNLOADED;

    res.name = (char*)allocator->malloc(name.size());
    //strcpy(res.name, name.c_str());
    res.name = name;

    if (filename != "") {
        const std::string& dir = findDirectory(filename);
        res.path = dir + filename;
    }
    else {
        res.path = "";
    }

    res.type = type;
    res.subtype = subtype;

    res.size = 0;
    res.data = 0;

    return res;
}

Resource& ResourceSystem::getResource(ResourceID id) {
    return resMap.at(id);
}

void ResourceSystem::rebuildPipelines() {
    for (auto& p : pipelines) {
        p.second.rebuild(toolchain);
    }
}

const std::string& ResourceSystem::findDirectory(const std::string& filename) {
    uint32_t size = directories.size();
    for (uint32_t i = 0; i < size; i++) {
        if (std::filesystem::exists(directories[i] + filename)) {
            return directories[i];
        }
    }

    std::cout << "Directory not found: " << filename << std::endl;
    assert(false, "Directory not found!");
    return 0;
}

void ResourceSystem::requestResourceLoad(ResourceID id) {
    assert(queuetop != queuebot - 1 || (queuetop == MAX_QUEUE - 1 && queuebot == 0), "Queue overflow");
    loadQueue[queuetop] = id;
    queuetop++;
    if (queuetop == MAX_QUEUE) queuetop = 0;
}

}