#pragma once

#include "edl/vk/vulkan.h"
#include "NewShader.h"
#include "ShaderReflection.h"
#include "GlobalInfo.h"
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

class FileRecord {
public:
    std::string name;
    std::string type; //Enum?
    std::string subtype; //Enum?
    std::string filename; //This should probably be saved as a full path
};

class Pipeline;
class ResourceSystem;

typedef void (*LoadFunction)(edl::res::Toolchain& toolchain, edl::res::Resource& res);

class ResourceLoader {
public:
    ResourceLoader() {}

    virtual void loadResource(ResourceSystem& system, const FileRecord& record) {};
};

// Goal 1: Converge DescriptorManager, StagingBuffer, ImageTable, and UniformBufferObject
// COMPLETE: Goal 2: Loading the individual segments (renderpass, shader, etc.) should load from the json or from another file
// Goal 3: Caching
class ResourceSystem {
public:
    ResourceSystem();
    ~ResourceSystem();

    //TODO: const-correctness, need to fix in Instance
    void init(vk::Instance& instance, global_info* globalInfo);

    void loadScene(const std::string& filename);

    void loadFiles(const rapidjson::GenericArray<false, rapidjson::Value>& files);

    void loadReflections(const rapidjson::GenericArray<false, rapidjson::Value>& reflections);
    void loadReflection(const rapidjson::GenericObject<false, rapidjson::Value>& reflection);

    void loadObject(const rapidjson::GenericObject<false, rapidjson::Value>& object);
    void loadModel(const rapidjson::GenericObject<false, rapidjson::Value>& object);
    void loadMaterial(const rapidjson::GenericObject<false, rapidjson::Value>& object);

    void draw(VkCommandBuffer& cb, uint32_t imageIndex);

    void registerLoadFunction(const std::string& type, LoadFunction function);

    res::Resource& createResource(std::string name, std::string filename, std::string type, std::string subtype);
    
    res::Resource& getResource(res::ResourceID id);
    inline bool resourceExists(res::ResourceID id) {
        return resMap.find(id) != resMap.end();
    }
    inline bool resourceLoaded(res::ResourceID id) {
        return resMap.find(id) != resMap.end() && resMap.at(id).status == res::ResourceStatus::LOADED;
    }

    const char* findDirectory(const char* filename);

    void requestResourceLoad(res::ResourceID id);

    void update(float delta);

    void drawMesh(VkCommandBuffer cb, VkPipelineLayout layout, res::ResourceID id);

    global_info* globalInfo;

    //TODO: Switch to proper tables
    std::unordered_map<std::string, ShaderHandle> shader_handles;
    std::unordered_map<std::string, ShaderReflection> reflections;

    std::unordered_map<std::string, Pipeline> pipelines;
    std::unordered_map<std::string, ImageHandle> images;

    std::unordered_map<uint32_t, DescriptorSetLayout> layouts;

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

    ShaderCompiler shader_compiler;

    std::vector<DescriptorHandle> imageHandles;

    mem::Allocator* allocator;

    std::unordered_map<res::ResourceID, res::Resource> resMap;

    std::unordered_map<res::ResourceType, LoadFunction> loadFunctionRegistry;

    static const uint32_t MAX_QUEUE = 64 * 1024;
    uint32_t queuebot = 0;
    uint32_t queuetop = 0;
    res::ResourceID loadQueue[MAX_QUEUE];

    BindlessImageDescriptor bindlessImageDescriptor;
    StorageBuffer transformBuffer;
    StorageBuffer materialBuffer;
    StorageBuffer drawDataBuffer;

    StorageBuffer sceneDataBuffer;
    StorageBuffer lightBuffer;

    StorageBuffer positionBuffer;
    StorageBuffer normalBuffer;
    StorageBuffer texCoord0Buffer;

    StorageBuffer indexBuffer;
    StorageBuffer indirectBuffer;

    std::unordered_map<std::string, int32_t> textureMap;

    res::Toolchain toolchain;

    Camera camera;
    glm::mat4 proj;
    GameObject root;
    std::vector<GameObject> objects;
    std::vector<Renderable> renderables;

private:
    

    

    
};

}