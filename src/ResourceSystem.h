#pragma once

#include "edl/vk/vulkan.h"
#include "edl/shader.h"
#include "edl/resource.h"
#include "descriptor_manager.h"
#include "StagingBuffer.h"
#include "UniformBufferObject.h"
#include "image_table.h"
#include "Model.h"
#include "bindless_image_buffer.h"
#include "StorageBuffer.h"
#include "ResourceHandles.h"
#include "Mesh.h"
#include "camera.h"
#include "GameObject.h"
#include "Renderable.h"
#include "bindless_types.h"
#include "pipeline.h"

#include "vulkan/vulkan.h"

#include <functional>
#include <string>
#include <vector>

namespace edl {

struct resource_record {

};

class ShaderHandle {
public:
    std::string name;
    VkShaderModule shader;
    VkShaderStageFlagBits stage;
};

class ResourceSystem;
class Pipeline;

// Goal 1: Converge DescriptorManager, StagingBuffer, ImageTable, and UniformBufferObject
// COMPLETE: Goal 2: Loading the individual segments (renderpass, shader, etc.) should load from the json or from another file
// Goal 3: Caching
class ResourceSystem {
public:
    ResourceSystem();
    ~ResourceSystem();

    //TODO: const-correctness, need to fix in Instance
    void init();

    void draw(VkCommandBuffer& cb, uint32_t imageIndex);

    void registerLoadFunction(const std::string& type, LoadFunction function);

    Resource& createResource(const std::string& name, const std::string& filename, const std::string& type, const std::string& subtype);
    
    Resource& getResource(ResourceID id);
    inline bool resourceExists(ResourceID id) {
        return resMap.find(id) != resMap.end();
    }
    inline bool resourceLoaded(ResourceID id) {
        return resMap.find(id) != resMap.end() && resMap.at(id).status == ResourceStatus::LOADED;
    }

    void rebuildPipelines();

    const std::string& ResourceSystem::findDirectory(const std::string& filename);

    void requestResourceLoad(ResourceID id);

    void update(float delta);

    void buildRenderInfo();

    //TODO: Switch to proper tables
    std::unordered_map<std::string, ShaderHandle> shader_handles;

    std::unordered_map<std::string, edl::Pipeline> pipelines;
    std::unordered_map<std::string, ImageHandle> images;

    std::unordered_map<uint32_t, DescriptorSetLayout> layouts;

    //TODO: PLEASE!!!
    VkPipelineLayout pipelineLayout;

    //TODO: Merge
    ImageTable imageTable;
    DescriptorManager descriptorManager;
    StagingBuffer stagingBuffer;
    UniformBufferObject uniformBufferObject;
    UniformBufferObject textureIndexObject;

    VkQueue queue;

    std::vector<std::string> directories;

    vk::Instance instance;
    VkPhysicalDevice vulkan_physical_device;
    VkDevice vulkan_device;
    vk::Swapchain swapchain;

    std::vector<VkImageView> imageviews;
    std::vector<VkRenderingAttachmentInfoKHR> attachments;
    std::vector<VkRenderingInfo> renderingInfos;

    std::vector<DescriptorHandle> imageHandles;

    DepthHandle depthHandle[2];

    Allocator* allocator;

    std::unordered_map<ResourceID, Resource> resMap;

    std::unordered_map<std::string, LoadFunction> loadFunctionRegistry;

    static const uint32_t MAX_QUEUE = 64 * 1024;
    uint32_t queuebot = 0;
    uint32_t queuetop = 0;
    ResourceID loadQueue[MAX_QUEUE];

    BindlessImageDescriptor bindlessImageDescriptor;
    StorageBuffer transformBuffer;
    StorageBuffer materialBuffer;
    StorageBuffer materialSetBuffer;
    StorageBuffer drawDataBuffer;

    StorageBuffer sceneDataBuffer;
    StorageBuffer lightBuffer;

    StorageBuffer geometryBuffer;

    StorageBuffer indexBuffer;
    StorageBuffer indirectBuffer;

    std::unordered_map<std::string, int32_t> textureMap;

    Toolchain toolchain;

    Camera camera;
    glm::mat4 proj;
    DirLight dirLight;

    ObjectRegistry objectRegistry;
    std::vector<Renderable> renderables;

    uint32_t width, height;

private:
    

    

    
};

}