#include "SceneLoader.h"

#include "JSONLoader.h"
#include "ResourceSystem.h"

#include "edl/io.h"
#include "edl/util.h"

#include "json.h"
#include "glm/glm.hpp"

#include <assert.h>
#include <iostream>

namespace edl {

void loadOptions(Toolchain& toolchain, Resource& res, void* obj) {
    ResourceSystem& system = toolchain.getTool<ResourceSystem>("system");
    json_object_s* o = json_value_as_object(static_cast<json_value_s*>(obj));

    json_object_element_s* e = o->start;
   
    while (e != nullptr) {
        if (json_string_as_cstring(e->name) == "Directories") {
            json_array_s* a = json_value_as_array(e->value);
            json_array_element_s* ae = a->start;
            while (ae != nullptr) {
                system.directories.push_back(json_value_as_string(ae->value)->string);
                ae = ae->next;
            }
        }

        e = e->next;
    }
}

void loadDirLight(Toolchain& toolchain, Resource& res, void* obj) {
    ResourceSystem& system = toolchain.getTool<ResourceSystem>("system");
    json_object_s* o = json_value_as_object(static_cast<json_value_s*>(obj));

    json_object_element_s* e = o->start;

    while (e != nullptr) {
        if (json_string_as_cstring(e->name) == "Direction") {
            system.dirLight.lightDir = json_value_as_vec4(e->value);
        }
        if (json_string_as_cstring(e->name) == "Color") {
            system.dirLight.lightColor = json_value_as_vec4(e->value);
        }
        if (json_string_as_cstring(e->name) == "Power") {
            system.dirLight.directionalLightPower = json_value_as_float(e->value);
        }

        e = e->next;
    }
}

void loadFiles(Toolchain& toolchain, Resource& res, void* obj) {
    ResourceSystem& system = toolchain.getTool<ResourceSystem>("system");
    json_array_s* a = json_value_as_array(static_cast<json_value_s*>(obj));
    json_array_element_s* ae = a->start;

    while (ae != nullptr) {
        json_object_s* o = json_value_as_object(ae->value);
        json_object_element_s* e = o->start;

        std::string name, type, subtype = "", filename;

        while (e != nullptr) {
            if (json_string_as_cstring(e->name) == "Name") {
                name = json_value_as_string(e->value)->string;
            }
            if (json_string_as_cstring(e->name) == "Type") {
                type = json_value_as_string(e->value)->string;
            }
            if (json_string_as_cstring(e->name) == "Subtype") {
                subtype = json_value_as_string(e->value)->string;
            }
            if (json_string_as_cstring(e->name) == "Filename") {
                filename = json_value_as_string(e->value)->string;
            }

            e = e->next;
        }

        assert(name != "" && type != "" && filename != "");

        Resource& r = system.createResource(name, filename, type, subtype);
        system.requestResourceLoad(r.id);

        ae = ae->next;
    }
}

void loadObjects(Toolchain& toolchain, Resource& res, void* obj) {
    ResourceSystem& system = toolchain.getTool<ResourceSystem>("system");
    json_array_s* a = json_value_as_array(static_cast<json_value_s*>(obj));
    json_array_element_s* ae = a->start;

    while (ae != nullptr) {
        json_object_s* o = json_value_as_object(ae->value);
        json_object_element_s* e = o->start;

        std::string name, model;
        glm::vec3 pos, scale;
        glm::quat rot;

        while (e != nullptr) {
            if (json_string_as_cstring(e->name) == "Name") {
                name = json_value_as_string(e->value)->string;
            }
            if (json_string_as_cstring(e->name) == "Position") {
                pos = json_value_as_vec3(e->value);
            }
            if (json_string_as_cstring(e->name) == "Rotation") {
                rot = json_value_as_euler(e->value);
            }
            if (json_string_as_cstring(e->name) == "Scale") {
                scale = json_value_as_vec3(e->value);
            }
            if (json_string_as_cstring(e->name) == "Model") {
                model = json_value_as_string(e->value)->string;
            }

            e = e->next;
        }

        GameObject* optr;
        if (system.objectRegistry.hasObject(name)) {
            optr = &system.objectRegistry.getObject(name);
        }
        else {
            optr = &system.objectRegistry.createObject(name);;
        }
        GameObject& go = *optr;

        go.position = pos;
        go.rotation = rot;
        go.scale = scale;

        go.calculateTransform();

        if (go.components.find("Renderable") == go.components.end()) {
            system.renderables.push_back({});
            Renderable& r = system.renderables.back();
            r.mvpHandle = getStorageBufferIndex(system.transformBuffer);
            r.parent = &go;
            go.components.insert({ "Renderable", &r });
        }

        Renderable& r = *(Renderable*)go.components.at("Renderable");
        edl::ResourceID meshID = edl::hash(model);
        r.model = meshID;

        ae = ae->next;
    }
}

void loadModels(Toolchain& toolchain, Resource& res, void* obj) {
    ResourceSystem& system = toolchain.getTool<ResourceSystem>("system");
    json_array_s* a = json_value_as_array(static_cast<json_value_s*>(obj));
    json_array_element_s* ae = a->start;

    while (ae != nullptr) {
        json_object_s* o = json_value_as_object(ae->value);
        json_object_element_s* e = o->start;

        std::string name, mesh, material;

        while (e != nullptr) {
            if (json_string_as_cstring(e->name) == "Name") {
                name = json_value_as_string(e->value)->string;
            }
            if (json_string_as_cstring(e->name) == "Mesh") {
                mesh = json_value_as_string(e->value)->string;
            }
            if (json_string_as_cstring(e->name) == "Material") {
                material = json_value_as_string(e->value)->string;
            }

            e = e->next;
        }

        Resource& r = system.createResource(name, "", "model", "");
        edl::allocateResourceData(r, sizeof(Model), *system.allocator);
        Model& model = edl::getResourceData<Model>(r);

        model.mesh = edl::hash(mesh);
        model.material = edl::hash(material);

        r.status = edl::ResourceStatus::LOADED;

        ae = ae->next;
    }
}

void loadMaterials(Toolchain& toolchain, Resource& res, void* obj) {
    ResourceSystem& system = toolchain.getTool<ResourceSystem>("system");
    json_array_s* a = json_value_as_array(static_cast<json_value_s*>(obj));
    json_array_element_s* ae = a->start;

    while (ae != nullptr) {
        json_object_s* o = json_value_as_object(ae->value);
        json_object_element_s* e = o->start;

        std::string name, albedo, normal;
        glm::vec4 tint;
        float metallic, roughness, ao;

        while (e != nullptr) {
            if (json_string_as_cstring(e->name) == "Name") {
                name = json_value_as_string(e->value)->string;
            }
            if (json_string_as_cstring(e->name) == "AlbedoTexture") {
                albedo = json_value_as_string(e->value)->string;
            }
            if (json_string_as_cstring(e->name) == "Normal") {
                normal = json_value_as_string(e->value)->string;
            }

            if (json_string_as_cstring(e->name) == "Tint") {
                tint = json_value_as_vec4(e->value);
            }
            if (json_string_as_cstring(e->name) == "Metallic") {
                metallic = json_value_as_float(e->value);
            }
            if (json_string_as_cstring(e->name) == "Roughness") {
                roughness = json_value_as_float(e->value);
            }
            if (json_string_as_cstring(e->name) == "AO") {
                ao = json_value_as_float(e->value);
            }

            e = e->next;
        }

        Resource& r = system.createResource(name, "", "material", "");
        edl::allocateResourceData(r, sizeof(PBRMaterial), *system.allocator);
        PBRMaterial& material = edl::getResourceData<PBRMaterial>(r);

        material.tint = tint;
        material.metallic = metallic;
        material.roughness = roughness;
        material.ao = ao;
        material.albedoTexture = edl::hash(albedo);
        material.normalTexture = edl::hash(normal);

        if (albedo == "") material.albedoTexture = 0;
        if (normal == "") material.normalTexture = 0;

        material.materialIndex = edl::getStorageBufferIndex(system.materialBuffer, 1);

        r.status = edl::ResourceStatus::LOADED;

        ae = ae->next;
    }
}

inline VkFormat parseFormat(const std::string& format, VkFormat swapchain) {
    if (format == "swapchain") {
        return swapchain;
    }
    if (format == "depth") {
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    }
}

void loadPipelines(Toolchain& toolchain, Resource& res, void* obj) {
    ResourceSystem& system = toolchain.getTool<ResourceSystem>("system");
    vk::Swapchain& swapchain = toolchain.getTool<vk::Swapchain>("VulkanSwapchain");
    json_array_s* a = json_value_as_array(static_cast<json_value_s*>(obj));
    json_array_element_s* ae = a->start;

    while (ae != nullptr) {
        json_object_s* o = json_value_as_object(ae->value);
        json_object_element_s* e = o->start;

        assert(json_string_as_cstring(e->name) == "Name");
        std::string name = json_value_as_string(e->value)->string;

        system.pipelines.insert({ name, {} });
        Pipeline& pipeline = system.pipelines.at(name);
        pipeline.setBindPoint(Pipeline::BindPoint::GRAPHICS);

        json_array_s* a2;
        json_array_element_s* ae2;

        e = e->next;
        assert(json_string_as_cstring(e->name) == "ColorFormats");

        a2 = json_value_as_array(e->value);
        ae2 = a2->start;

        while (ae2 != nullptr) {
            std::string format = json_value_as_string(ae2->value)->string;
            pipeline.setColorFormat(parseFormat(format, swapchain.format));

            ae2 = ae2->next;
        }

        e = e->next;
        assert(json_string_as_cstring(e->name) == "DepthStencilFormat");

        std::string format = json_value_as_string(e->value)->string;
        pipeline.setDepthFormat(parseFormat(format, swapchain.format));

        e = e->next;
        assert(json_string_as_cstring(e->name) == "Shaders");

        a2 = json_value_as_array(e->value);
        ae2 = a2->start;

        while (ae2 != nullptr) {
            std::string shader = json_value_as_string(ae2->value)->string;
            pipeline.addShader(shader);

            ae2 = ae2->next;
        }

        ae = ae->next;
    }
    
    //TODO: Actually do anything

}

void initSceneLoader(Toolchain& toolchain) {
    if (!toolchain.has("JSONLoader")) return;

    JSONLoader& loader = toolchain.getTool<JSONLoader>("JSONLoader");
    JSONSubTypeLoader& subloader = loader.createSubTypeLoader("Scene");

    //subloader.setLoadFunction(loadScene);
    subloader.addLoadFunction("Options", loadOptions);
    subloader.addLoadFunction("Load", loadFiles);
    subloader.addLoadFunction("DirLight", loadDirLight);
    subloader.addLoadFunction("Objects", loadObjects);
    subloader.addLoadFunction("Models", loadModels);
    subloader.addLoadFunction("Materials", loadMaterials);
    subloader.addLoadFunction("Pipelines", loadPipelines);
}

}