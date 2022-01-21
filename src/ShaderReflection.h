#pragma once

#include "vulkan/vulkan.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace edl {

enum class glsl_datatype : uint32_t {
    uint32_t,
    vec2,
    vec3,
    vec4,
    mat4,
    sampler2D,
    sampler,
    texture2D,

    unimplemented,

    //Unsized types
    ubo,
    buffer,

    NUM_DATATYPES,
};

static const size_t DATATYPE_SIZES[static_cast<uint32_t>(glsl_datatype::NUM_DATATYPES)] = {
    sizeof(uint32_t),   //uint32_t
    sizeof(float) * 2,  //vec2
    sizeof(float) * 3,  //vec3
    sizeof(float) * 4,  //vec4
    sizeof(float) * 16, //mat4
    sizeof(VkSampler),  //sampler2D
    sizeof(VkSampler),  //sampler
    sizeof(VkImage),    //texture2D
    SIZE_MAX            //unimplemented
};

static const std::unordered_map<std::string, glsl_datatype> DATATYPE_MAP = {
    {"uint32_t",  glsl_datatype::uint32_t},
    {"vec2",      glsl_datatype::vec2},
    {"vec3",      glsl_datatype::vec3},
    {"vec4",      glsl_datatype::vec4},
    {"mat4",      glsl_datatype::mat4},
    {"sampler2D", glsl_datatype::sampler2D},
    {"sampler",   glsl_datatype::sampler},
    {"texture2D", glsl_datatype::texture2D},
    {"ubo",       glsl_datatype::ubo},
    {"buffer",    glsl_datatype::buffer}
};

static const std::unordered_map<glsl_datatype, VkFormat> DATATYPE_FORMAT = {
    {glsl_datatype::uint32_t,  VK_FORMAT_R32_SFLOAT},
    {glsl_datatype::vec2,      VK_FORMAT_R32G32_SFLOAT},
    {glsl_datatype::vec3,      VK_FORMAT_R32G32B32_SFLOAT},
    {glsl_datatype::vec4,      VK_FORMAT_R32G32B32A32_SFLOAT}
};

class ShaderReflection {
public:
    ShaderReflection() {}

    void init(const rapidjson::GenericObject<false, rapidjson::Value>& reflection, const VkShaderStageFlagBits& shader_stage);

    class Attribute {
    public:
        std::string   name;
        VkShaderStageFlagBits stage;
        glsl_datatype datatype;
        VkFormat      format;
        size_t        size;
        uint32_t        location;

        bool operator==(const Attribute& attr) const {
            return this->name     == attr.name &&
                   this->datatype == attr.datatype &&
                   this->format   == attr.format &&
                   this->size     == attr.size &&
                   this->location == attr.location;
        }
    };

    class UniformBufferObjectField {
    public:
        std::string   name;
        glsl_datatype datatype;
        size_t        size;
        size_t        offset;
        uint32_t      count;

        bool operator==(const UniformBufferObjectField& field) const {
            return this->name == field.name &&
                this->datatype == field.datatype &&
                this->size == field.size &&
                this->offset == field.offset &&
                this->count == field.count;
        }
    };

    class Uniform {
    public:
        std::string   name;
        VkShaderStageFlagBits stage;
        glsl_datatype datatype;
        size_t        size;
        uint32_t      set;
        uint32_t      binding;
        uint32_t      count;
        std::vector<UniformBufferObjectField> fields;

        bool operator==(const Uniform& unif) const {
            if (fields.size() != unif.fields.size()) return false;
            for (uint32_t i; i < fields.size(); ++i) {
                if (fields[i] == unif.fields[i]) continue;
                return false;
            }
            return this->name     == unif.name &&
                   this->datatype == unif.datatype &&
                   this->size     == unif.size &&
                   this->set      == unif.set &&
                   this->binding  == unif.binding &&
                   this->count    == unif.count;
        }
    };

    ShaderReflection operator+(const ShaderReflection& refl);
    ShaderReflection operator+=(const ShaderReflection& refl) { return this->operator+(refl); }

    //Attribute& getAttribute(const std::string& name);
    //Uniform& getUniform(const std::string& name);
    //UniformBufferObject& getUniformBufferObject(const std::string& name);

    VkShaderStageFlagBits shaderStage;

    std::vector<Attribute> attributes;
    std::unordered_map<uint32_t, std::vector<Uniform>> uniforms;

private:
    //std::unordered_map<std::string, size_t> attribute_map;
    //std::unordered_map<std::string, size_t> uniform_map;
    //std::unordered_map<std::string, size_t> uniform_buffer_object_map;
};

}