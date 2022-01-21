#include "ShaderReflection.h"

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <iostream>
#include <unordered_set>

namespace edl {

void ShaderReflection::init(const rapidjson::GenericObject<false, rapidjson::Value>& reflection, const VkShaderStageFlagBits& shader_stage) {
    // Set shaderStage
    this->shaderStage = shader_stage;

    // Load Attributes
    if (reflection.HasMember("Attributes")) {
        auto& attributes = reflection["Attributes"].GetArray();
        for (auto* ptr = attributes.Begin(); ptr != attributes.End(); ++ptr) {
            auto& attribute = ptr->GetObject();
            this->attributes.push_back({});
            Attribute& attr = this->attributes.back();

            // Load Name
            assert(attribute.HasMember("Name"));
            attr.name = attribute["Name"].GetString();

            // Set Shader Stage
            attr.stage = shader_stage;

            // Load Location
            assert(attribute.HasMember("Location"));
            attr.location = attribute["Location"].GetUint();

            // Load Datatype
            assert(attribute.HasMember("Datatype"));
            std::string datatype = attribute["Datatype"].GetString();
            if (DATATYPE_MAP.find(datatype) == DATATYPE_MAP.end()) {
                attr.datatype = glsl_datatype::unimplemented;
            }
            else {
                attr.datatype = DATATYPE_MAP.at(datatype);
            }

            // Calculate vulkan format
            attr.format = DATATYPE_FORMAT.at(attr.datatype);

            // Calculate size
            attr.size = DATATYPE_SIZES[static_cast<uint32_t>(attr.datatype)];

            // Add to map
            //attribute_map[attr.name] = this->attributes.size() - 1;
        }
    }

    // Load Uniforms
    if (reflection.HasMember("Uniforms")) {
        auto& uniforms = reflection["Uniforms"].GetArray();
        for (auto* ptr = uniforms.Begin(); ptr != uniforms.End(); ++ptr) {
            auto& uniform = ptr->GetObject();

            // Load Set
            assert(uniform.HasMember("Set"));
            uint32_t set = uniform["Set"].GetUint();

            this->uniforms[set].push_back({});
            Uniform& unif = this->uniforms[set].back();

            // Load Name
            assert(uniform.HasMember("Name"));
            unif.name = uniform["Name"].GetString();

            // Set Shader Stage
            unif.stage = shader_stage;

            // Set set
            unif.set = set;

            // Load Binding
            assert(uniform.HasMember("Binding"));
            unif.binding = uniform["Binding"].GetUint();

            // Load Datatype
            assert(uniform.HasMember("Datatype"));
            std::string datatype = uniform["Datatype"].GetString();
            if (DATATYPE_MAP.find(datatype) == DATATYPE_MAP.end()) {
                unif.datatype = glsl_datatype::unimplemented;
            }
            else {
                unif.datatype = DATATYPE_MAP.at(datatype);
            }

            // Account for Uniform Buffer Objects
            if (datatype == "ubo") {
                // Load Fields
                assert(uniform.HasMember("Fields"));
                size_t size = 0;
                auto& fields = uniform["Fields"].GetArray();
                for (auto* ptr0 = fields.Begin(); ptr0 != fields.End(); ++ptr0) {
                    auto& field = ptr0->GetObject();
                    unif.fields.push_back({});
                    UniformBufferObjectField& f = unif.fields.back();

                    // Load Name
                    assert(field.HasMember("Name"));
                    f.name = field["Name"].GetString();

                    // Load Datatype
                    assert(field.HasMember("Datatype"));
                    std::string datatype = field["Datatype"].GetString();
                    if (DATATYPE_MAP.find(datatype) == DATATYPE_MAP.end()) {
                        f.datatype = glsl_datatype::unimplemented;
                    }
                    else {
                        f.datatype = DATATYPE_MAP.at(datatype);
                    }

                    // Calculate size
                    f.size = DATATYPE_SIZES[static_cast<uint32_t>(f.datatype)];

                    // Calculate field array count
                    f.count = 1;
                    if (field.HasMember("Count")) {
                        f.count = field["Count"].GetUint64();
                    }


                    // Calculate offset
                    f.offset = size;

                    // Add size to total UBO size
                    size += f.size * f.count;
                }

                // Calculate uniform array count
                unif.count = 1;

                // Calculate size from fields
                unif.size = size;
            }
            else {
                //Calculate uniform array count
                unif.count = 1;
                if (uniform.HasMember("Count")) {
                    unif.count = uniform["Count"].GetUint64();
                }

                // Calculate size using datatype
                unif.size = DATATYPE_SIZES[static_cast<uint32_t>(unif.datatype)];
            }

            if (datatype == "buffer") {
                unif.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_ALL;
            }

            // Add to map
            //uniform_map[unif.name] = this->uniforms.size() - 1;
        }
    }
}

ShaderReflection ShaderReflection::operator+(const ShaderReflection& refl) {
    ShaderReflection out;

    //TODO: It was late when I wrote this, do something better
    // Attributes
    std::unordered_map<size_t, uint32_t> binding_map;
    for (const Attribute& a : attributes) {
        binding_map[a.location] = out.attributes.size();
        out.attributes.push_back(Attribute(a));
    }
    for (const Attribute& a : refl.attributes) {
        if (binding_map.find(a.location) == binding_map.end()) {
            out.attributes.push_back(Attribute(a)); // Don't bother updating the binding_map
        }
        else {
            Attribute& attribute = out.attributes[binding_map.at(a.location)];
            if (a == attribute) {
                attribute.stage = static_cast<VkShaderStageFlagBits>(attribute.stage | a.stage);
                continue;
            }
            std::cout << "Shader Reflection Addition: Shader attribute not compatible!"                  << std::endl;
            std::cout << "Left--> Attribute: " << attribute.name << ", Location: " << attribute.location << std::endl;
            std::cout << "Right-> Attribute: " << a.name         << ", Location: " << a.location         << std::endl;
        }
    }

    // Uniforms
    binding_map.clear();
    for (auto& ptr = uniforms.begin(); ptr != uniforms.end(); ++ptr) {
        for (const Uniform& u : ptr->second) {
            binding_map[u.binding] = out.uniforms.size();
            out.uniforms[ptr->first].push_back(Uniform(u));
        }
    }
    for (auto& ptr = refl.uniforms.begin(); ptr != refl.uniforms.end(); ++ptr) {
        for (const Uniform& u : ptr->second) {
            bool yes = true;
            for (auto& u2 : out.uniforms[ptr->first]) {
                if (u2.binding == u.binding) {
                    yes = false;
                    break;
                }
            }

            if (yes) {
                out.uniforms[ptr->first].push_back(Uniform(u));
            }

            //if (binding_map.find(u.binding) == binding_map.end() && out.uniforms[ptr->first].size() > binding_map.at(u.binding)) {
            //    out.uniforms[ptr->first].push_back(Uniform(u)); // Don't bother updating the binding_map
            //}
            //else {
            //    Uniform& uniform = out.uniforms[ptr->first][binding_map.at(u.binding)];
            //    if (u == uniform) {
            //        uniform.stage = static_cast<VkShaderStageFlagBits>(uniform.stage | u.stage);
            //        continue;
            //    }
            //    std::cout << "Shader Reflection Addition: Shader uniform not compatible!" << std::endl;
            //    std::cout << "Left--> Uniform: " << uniform.name << ", Set: " << uniform.set << ", Binding: " << uniform.binding << std::endl;
            //    std::cout << "Right-> Uniform: " << u.name << ", Set: " << uniform.set << ", Binding: " << u.binding << std::endl;
            //}
        }
    }

    out.shaderStage = static_cast<VkShaderStageFlagBits>(this->shaderStage | refl.shaderStage);

    return out;
}
/*
ShaderReflection::Attribute& ShaderReflection::getAttribute(const std::string& name) {
    return attributes[attribute_map.at(name)];
}

ShaderReflection::Uniform& ShaderReflection::getUniform(const std::string& name) {
    return uniforms[uniform_map.at(name)];
}

ShaderReflection::UniformBufferObject& ShaderReflection::getUniformBufferObject(const std::string& name) {
    return uniformBufferObjects[uniform_buffer_object_map.at(name)];
}
*/
}