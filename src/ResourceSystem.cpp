#include "ResourceSystem.h"

#include "pipeline.h"
#include "Util.h"
#include "ShaderLoader.h"
#include "bindless_types.h"
#include "edl/debug.h"
#include "edl/io.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace edl {

ResourceSystem::ResourceSystem() {
    allocator = new mem::DefaultAllocator();
}

ResourceSystem::~ResourceSystem() {

}

void ResourceSystem::init(vk::Instance& instance, global_info* globalInfo) {
    this->instance = instance;
    vulkan_physical_device = instance.physicalDevice;
    vulkan_device = instance.device;

    this->globalInfo = globalInfo;

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(vulkan_physical_device, &props);

    stagingBuffer.create(vulkan_physical_device, vulkan_device, 256 * 1028 * 1028, 0);
    imageTable.init(vulkan_physical_device, vulkan_device, 1920 * 1080 * 4 * 20);
    uniformBufferObject.create(vulkan_physical_device, vulkan_device, 20000, sizeof(float) * 16, props.limits.minUniformBufferOffsetAlignment);
    textureIndexObject.create(vulkan_physical_device, vulkan_device, 20000, sizeof(DrawData), props.limits.minUniformBufferOffsetAlignment);
    descriptorManager.init(vulkan_device);

    //TODO: Come on vulkan >.<
    VkBuffer wasteOfMemory = vk::createBuffer(vulkan_device, 1, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    uint32_t objectMax = 30000;
    uint32_t triangleMax = 64 * 1024 * 1024;
    uint32_t materialMax = 10000;

    bindlessImageDescriptor.create(instance.device, 0, materialMax);

    transformBuffer = createStorageBuffer(instance, sizeof(glm::mat4), objectMax);
    materialBuffer = createStorageBuffer(instance, sizeof(Material), 10000);
    drawDataBuffer = createStorageBuffer(instance, sizeof(DrawData), objectMax * 5);

    positionBuffer = createStorageBuffer(instance, sizeof(glm::vec4), triangleMax, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    normalBuffer = createStorageBuffer(instance, sizeof(glm::vec4), triangleMax, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    texCoord0Buffer = createStorageBuffer(instance, sizeof(glm::vec2), triangleMax, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    indexBuffer = createStorageBuffer(instance, sizeof(uint16_t), triangleMax * 3, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    indirectBuffer = createStorageBuffer(instance, sizeof(VkDrawIndexedIndirectCommand), objectMax, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);

    sceneDataBuffer = createStorageBuffer(instance, sizeof(SceneData), 1);
    lightBuffer = createStorageBuffer(instance, sizeof(Light), 10);

    toolchain.add("system", this);

    //TODO: If something is messing up, it's probably this!
    objects.reserve(1000);
    renderables.reserve(1000);
}

void ResourceSystem::loadScene(const std::string& filename) {
    std::string dir = "./res/scene/";
    std::vector<char> data;
    loadFile(dir + filename, data);

    rapidjson::Document d;
    d.Parse<rapidjson::kParseStopWhenDoneFlag>(data.data(), data.size());
    if (d.HasParseError()) {
        std::cout << "JSON parsing error, code: " << d.GetParseError() << ", offset: " << d.GetErrorOffset() << std::endl;
    }
    assert(d.IsObject());

    if (d.HasMember("Options")) {
        auto& options = d["Options"].GetObject();

        if (options.HasMember("Directories")) {
            for (auto* ptr = options["Directories"].GetArray().Begin(); ptr != options["Directories"].GetArray().End(); ++ptr) {
                const char* dir = ptr->GetString();
                directories.push_back(dir);
            }
        }
    }

    if (d.HasMember("Load")) {
        loadFiles(d["Load"].GetArray());
    }

    update(0);//TODO: Get rid of this

    if (d.HasMember("ShaderReflection")) {
        loadReflections(d["ShaderReflection"].GetArray());
    }

    if (d.HasMember("Pipelines")) {
        for (auto* ptr = d["Pipelines"].GetArray().Begin(); ptr != d["Pipelines"].GetArray().End(); ++ptr) {
            auto& pipeline = ptr->GetObject();

            assert(pipeline.HasMember("Name"));
            std::string name = pipeline["Name"].GetString();

            if (pipelines.find(name) != pipelines.end()) return; //Already loaded

            //Load shaders
            //TODO: Better Reflection
            //TODO: Caching and stuff

            //TODO: MOVE Shaders and meshes into own sections

            //Secondly, only handles (which should be uint64_t) should ever leave this object
            // Everything should link to this and use it in combination with the handles
            // ie. getMesh(string name) => mesh_handle
            // drawMesh(mesh_handle handle) => drawMesh()
            // Might need a hybrid system where you return objects that can be used externally 

            // Files loaded by outside file types would have the same name as their filename

            //TODO: Async implementation
            std::vector<std::string> shaders; //TODO: This is kinda yikes
            assert(pipeline.HasMember("Shaders"));
            for (auto* ptr1 = pipeline["Shaders"].GetArray().Begin(); ptr1 != pipeline["Shaders"].GetArray().End(); ++ptr1) {
                shaders.push_back(ptr1->GetString());
            }


            //TODO: Handle multiple
            pipelines[name].init(pipeline, shaders, shader_handles, reflections, globalInfo);
            for (auto& ptr = pipelines[name].bindings.begin(); ptr != pipelines[name].bindings.end(); ++ptr) {
                layouts[ptr->first] = descriptorManager.createLayout(ptr->second);
            }
        }
    }

    if (d.HasMember("Objects")) {
        auto& models = d["Objects"].GetArray();
        for (auto* ptr = models.Begin(); ptr != models.End(); ++ptr) {
            loadObject(ptr->GetObject());
        }
    }

    if (d.HasMember("Models")) {
        auto& member = d["Models"].GetArray();
        for (auto* ptr = member.Begin(); ptr != member.End(); ++ptr) {
            loadModel(ptr->GetObject());
        }
    }

    if (d.HasMember("Materials")) {
        auto& member = d["Materials"].GetArray();
        for (auto* ptr = member.Begin(); ptr != member.End(); ++ptr) {
            loadMaterial(ptr->GetObject());
        }
    }

    if (d.HasMember("LoadLate")) {
        loadFiles(d["LoadLate"].GetArray());
    }

    if (d.HasMember("Scenes")) {
        for (auto* ptr1 = d["Scenes"].GetArray().Begin(); ptr1 != d["Scenes"].GetArray().End(); ++ptr1) {
            loadScene(ptr1->GetString());
        }
    }
    
}

void ResourceSystem::loadFiles(const rapidjson::GenericArray<false, rapidjson::Value>& files) {
    for (auto* ptr = files.Begin(); ptr != files.End(); ++ptr) {
        auto& obj = ptr->GetObject();

        assert(obj.HasMember("Name"));
        std::string name = obj["Name"].GetString();

        assert(obj.HasMember("Type"));
        std::string type = obj["Type"].GetString();

        std::string subtype = "";
        if (obj.HasMember("Subtype")) {
            subtype = obj["Subtype"].GetString();
        }

        assert(obj.HasMember("Filename"));
        std::string filename = obj["Filename"].GetString();

        res::Resource& res = createResource(name, filename, type, subtype);

        //TODO: Stop prioritizing shaders like this, pipelines need to be fixed first
        //if (type == "shader") {
        //    LoadFunction loadFunction = loadFunctionRegistry.at(res.type);
        //    loadFunction(*this, res);
        //}
        //else {
            requestResourceLoad(res.id);
        //}
    }
}

void ResourceSystem::loadReflections(const rapidjson::GenericArray<false, rapidjson::Value>& reflections) {
    for (auto* ptr = reflections.Begin(); ptr != reflections.End(); ++ptr) {
        loadReflection(ptr->GetObject());
    }
}

void ResourceSystem::loadReflection(const rapidjson::GenericObject<false, rapidjson::Value>& reflection) {
    assert(reflection.HasMember("Name"));
    std::string name = reflection["Name"].GetString();

    if (reflections.find(name) != reflections.end()) return; // If it is already loaded, don't

    VkShaderStageFlagBits stage;
    assert(reflection.HasMember("Type"));
    std::string type = reflection["Type"].GetString();
    if (type == "vert") stage = VK_SHADER_STAGE_VERTEX_BIT;
    else if (type == "frag") stage = VK_SHADER_STAGE_FRAGMENT_BIT;

    reflections[name].init(reflection, stage);
}

void ResourceSystem::loadObject(const rapidjson::GenericObject<false, rapidjson::Value>& object) {
    //TODO: Check if it's already loaded

    objects.push_back({});
    GameObject& o = objects.back();
    root.addChild(o);

    assert(object.HasMember("Name"));
    o.name = object["Name"].GetString();

    // Setup Position
    if (object.HasMember("Position")) {
        auto& arr = object["Position"].GetArray();
        o.position = glm::vec3(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat());
    }
    
    // Setup Rotation from gradian euler angles
    if (object.HasMember("Rotation")) {
        auto& arr = object["Rotation"].GetArray();
        o.rotation = glm::quat(glm::vec3(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat()));
    }

    // Setup Scale
    if (object.HasMember("Scale")) {
        auto& arr = object["Scale"].GetArray();
        o.scale = glm::vec3(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat());
    }

    o.calculateTransform();

    // Setup Model
    if (object.HasMember("Model")) {
        renderables.push_back({});
        Renderable& r = renderables.back();
        r.mvpHandle = getStorageBufferIndex(transformBuffer);
        r.parent = &o;
        std::string meshName = object["Model"].GetString();
        edl::res::ResourceID meshID = edl::hashString(meshName);
        r.model = meshID;

        o.component = &r;
    }
}

void ResourceSystem::loadModel(const rapidjson::GenericObject<false, rapidjson::Value>& object) {
    assert(object.HasMember("Name"));
    assert(object.HasMember("Mesh"));
    assert(object.HasMember("Material"));

    std::string name = object["Name"].GetString();
    std::string mesh = object["Mesh"].GetString();
    std::string material = object["Material"].GetString();

    res::Resource& res = createResource(name, "", "model", "");
    edl::res::allocateResourceData(res, sizeof(Model), *allocator);
    Model& model = edl::res::getResourceData<Model>(res);

    model.mesh = edl::hashString(mesh);
    model.material = edl::hashString(material);


    res.status = edl::res::ResourceStatus::LOADED;
}

void ResourceSystem::loadMaterial(const rapidjson::GenericObject<false, rapidjson::Value>& object) {
    assert(object.HasMember("Name"));
    assert(object.HasMember("Tint"));
    assert(object.HasMember("Metallic"));
    assert(object.HasMember("Roughness"));
    assert(object.HasMember("AO"));
    assert(object.HasMember("AlbedoTexture"));
    assert(object.HasMember("NormalTexture"));

    std::string name = object["Name"].GetString();

    res::Resource& res = createResource(name, "", "material", "");
    edl::res::allocateResourceData(res, sizeof(PBRMaterial), *allocator);
    PBRMaterial& material = edl::res::getResourceData<PBRMaterial>(res);

    auto& tintarr = object["Tint"].GetArray();
    material.tint.r = tintarr[0].GetFloat();
    material.tint.g = tintarr[1].GetFloat();
    material.tint.b = tintarr[2].GetFloat();
    material.tint.a = tintarr[3].GetFloat();


    material.metallic = object["Metallic"].GetFloat();
    material.roughness = object["Roughness"].GetFloat();
    material.ao = object["AO"].GetFloat();

    std::string albedo = object["AlbedoTexture"].GetString();
    std::string normal = object["NormalTexture"].GetString();

    material.albedoTexture = edl::hashString(albedo);
    material.normalTexture = edl::hashString(normal);

    if (albedo == "") material.albedoTexture = 0;
    if (normal == "") material.normalTexture = 0;

    material.materialIndex = edl::getStorageBufferIndex(materialBuffer, 1);

    res.status = edl::res::ResourceStatus::LOADED;
}

void ResourceSystem::draw(VkCommandBuffer& cb, uint32_t imageIndex) {
    std::string currentPipeline = "";
    
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)globalInfo->width;
    viewport.height = (float)globalInfo->height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    if (renderables.size() == 0) return;

    Pipeline& pipeline = pipelines["Shader3D"];
    VkRenderingInfoKHR& renderingInfo = pipeline.renderingInfos[imageIndex];

    PFN_vkCmdBeginRenderingKHR bRender = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(instance.device, "vkCmdBeginRenderingKHR"));
    bRender(cb, &renderingInfo);

    //edl::vk::vkCmdBeginRenderingKHR(cb, &pipeline.renderingInfos[imageIndex]);
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    vkCmdSetViewport(cb, 0, 1, &viewport);

    uint32_t o[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    VkDescriptorSet sets[9] = { 
        sceneDataBuffer.descriptorSet,
        drawDataBuffer.descriptorSet,
        transformBuffer.descriptorSet,
        positionBuffer.descriptorSet,
        normalBuffer.descriptorSet,
        texCoord0Buffer.descriptorSet,
        materialBuffer.descriptorSet,
        lightBuffer.descriptorSet,
        bindlessImageDescriptor.descriptorSet };
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipelineLayout, 0, 9, sets, 8, o);
    //
    ////TODO: Bind them all at the same time
    //vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[models[0].pipeline].pipelineLayout, 0, 1, &drawDataBuffer.descriptorSet, 1, &o);
    //vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[models[0].pipeline].pipelineLayout, 1, 1, &transformBuffer.descriptorSet, 1, &o);
    //vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[models[0].pipeline].pipelineLayout, 2, 1, &positionBuffer.descriptorSet, 1, &o);
    //vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[models[0].pipeline].pipelineLayout, 3, 1, &normalBuffer.descriptorSet, 1, &o);
    //vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[models[0].pipeline].pipelineLayout, 4, 1, &texCoord0Buffer.descriptorSet, 1, &o);
    //vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[models[0].pipeline].pipelineLayout, 5, 1, &texCoord1Buffer.descriptorSet, 1, &o);

    //vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[models[0].pipeline].pipelineLayout, 6, 1, &bindlessImageDescriptor.descriptorSet, 0, nullptr);

    vkCmdBindIndexBuffer(cb, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

    uint32_t drawID = 0;

    glm::vec3 campos = camera.getPos();

    SceneData scene = {};
    scene.activeLights = 0;
    scene.cameraPosition = glm::vec4(campos, 1.0f);

    edl::updateStorageBuffer(stagingBuffer, sceneDataBuffer, 0, &scene, 1);

    //TODO: Update lights

    for (int i = 0; i < renderables.size(); ++i) {

        // Update the transform
        //edl::updateStorageBuffer(stagingBuffer, transformBuffer, renderables[i].mvpHandle, &renderables[i].parent->transform, 1);

        //TODO: EVERYTHING NEEDS TO BE FIXED
        //models[i].draw(cb, pipelines[models[i].pipeline].pipelineLayout);
        //drawMesh(cb, pipelines[models[i].pipeline].pipelineLayout, models[i].mesh);

        res::Resource& res = resMap.at(renderables[i].model);
        Model& model = res::getResourceData<Model>(res);

        if (resMap.find(model.mesh) == resMap.end()) {
            std::cout << "Mesh not found for model: " << res.name << std::endl;
            continue;
        }

        if (resMap.find(model.material) == resMap.end()) {
            std::cout << "Material not found for model: " << res.name << std::endl;
            continue;
        }

        Mesh& mesh = res::getResourceData<Mesh>(resMap.at(model.mesh));
        PBRMaterial& material = res::getResourceData<PBRMaterial>(resMap.at(model.material));

        Material mat;// = material;
        mat.tint = material.tint;
        mat.metallic = material.metallic;
        mat.roughness = material.roughness;
        mat.ao = material.ao;

        if (material.albedoTexture > 0) {
            res::Resource& ires = resMap.at(material.albedoTexture);
            res::Image& image = res::getResourceData<res::Image>(ires);

            mat.albedoTexture = image.materialIndex;
        }
        else {
            mat.albedoTexture = -1;
        }

        if (material.normalTexture > 0) {
            res::Resource& ires = resMap.at(material.normalTexture);
            res::Image& image = res::getResourceData<res::Image>(ires);

            mat.normalTexture = image.materialIndex;
        }
        else {
            mat.normalTexture = -1;
        }

        DrawData drawData = {};
        drawData.positionOffset = mesh.positionOffset;
        drawData.normalOffset = mesh.normalOffset;
        drawData.tangentOffset = mesh.tangentOffset;
        drawData.texCoord0Offset = mesh.texCoord0Offset;
        drawData.materialOffset = model.material;
        drawData.mvpOffset = renderables[i].mvpHandle;

        drawData.materialOffset = material.materialIndex;
        edl::updateStorageBuffer(stagingBuffer, drawDataBuffer, drawID, &drawData, 1);

        edl::updateStorageBuffer(stagingBuffer, materialBuffer, material.materialIndex, &mat, 1);

        VkDrawIndexedIndirectCommand indirect = {};
        indirect.instanceCount = 1;
        indirect.vertexOffset = 0;
        indirect.firstIndex = mesh.indexOffset;
        indirect.indexCount = mesh.indexCount;
        indirect.firstInstance = 0;
        edl::updateStorageBuffer(stagingBuffer, indirectBuffer, drawID, &indirect, 1);

        drawID++;
    }

    vkCmdDrawIndexedIndirect(cb, indirectBuffer.buffer, 0, drawID, sizeof(VkDrawIndexedIndirectCommand));

    //vkCmdEndRenderingKHR(cb);
    edl::vk::vkCmdEndRenderingKHR(cb);
}

void ResourceSystem::registerLoadFunction(const std::string& type, LoadFunction function) {
    res::ResourceType restype = hashString(type);

    if (loadFunctionRegistry.find(restype) != loadFunctionRegistry.end()) {
        std::cout << "Loader already registered for type: " << type << std::endl;
    }

    loadFunctionRegistry.insert({ restype, function });
}

void ResourceSystem::update(float delta) {
    //fileLoader.loadChunks(100);
    while (queuebot != queuetop) {
        res::ResourceID id = loadQueue[queuebot];

        res::Resource& res = resMap.at(id);
        //TODO: Check requirements loaded
        if (res.status == res::ResourceStatus::UNLOADED) {
            LoadFunction loadFunction = loadFunctionRegistry.at(res.type);

            loadFunction(toolchain, res);
        }

        loadQueue[queuebot] = 0;
        queuebot++;
        if (queuebot == MAX_QUEUE) queuebot = 0;
    }

    root.update(toolchain, delta);
    //fileLoader.cleanup();
}

//Maybe multiple create functions, one for named resources, unnamed resources, and filenamed resources
res::Resource& ResourceSystem::createResource(std::string name, std::string filename, std::string type, std::string subtype) {
    uint64_t nameHash = hashString(name);
    uint64_t filenameHash = hashString(filename);

    res::ResourceID id = nameHash;//filenameHash == 0 ? nameHash : filenameHash;
    if (resMap.find(id) != resMap.end()) { //TODO: IDing needs to be fixed, need support for multiple resources loaded from the same file
        //printf("Resource already has name: %s\n", name.c_str());
        return resMap.at(id);
    }

    res::Resource& res = resMap[id];
    res.id = id;

    res.status = res::ResourceStatus::UNLOADED;

    res.nameHash = nameHash;
    res.name = (char*)allocator->malloc(name.size());
    strcpy(res.name, name.c_str());

    if (filename != "") {
        const char* dir = findDirectory(filename.c_str());
        size_t dirSize = strlen(dir);
        size_t filenameSize = filename.size();

        res.filenameHash = hashString(filename);
        res.path = (char*)allocator->malloc(dirSize + filenameSize);
        strcpy(res.path, dir);
        strcpy(res.path + dirSize, filename.c_str());
    }
    else {
        res.filenameHash = 0;
        res.path = 0;
    }

    res.type = hashString(type);
    res.subtype = hashString(subtype);

    res.dependenciesCount = 0;
    res.dependencies = 0;

    res.size = 0;
    res.data = 0;

    return res;
}

res::Resource& ResourceSystem::getResource(res::ResourceID id) {
    return resMap.at(id);
}

const char* ResourceSystem::findDirectory(const char* filename) {

    uint32_t size = directories.size();
    for (uint32_t i = 0; i < size; i++) {
        if (std::filesystem::exists(directories[i] + filename)) {
            return directories[i].c_str();
        }
    }

    assert(false, "Directory not found!");
}

void ResourceSystem::requestResourceLoad(res::ResourceID id) {
    assert(queuetop != queuebot - 1 || (queuetop == MAX_QUEUE - 1 && queuebot == 0), "Queue overflow");
    loadQueue[queuetop] = id;
    queuetop++;
    if (queuetop == MAX_QUEUE) queuetop = 0;
}

}