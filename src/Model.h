#pragma once

#include "glm/glm.hpp"

#include <stdint.h>

namespace edl {

struct MaterialSet {
    uint64_t materials[16];
};

struct Material {
    glm::vec4 tint;

    float metallic;
    float roughness;
    float ao;

    int32_t albedoTexture;
    int32_t normalTexture;
    //int roughnessTexture;
    int32_t pad0;
    int32_t pad1;
    int32_t pad2;
    //int pad3;
};

struct PBRMaterial {
    glm::vec4 tint;

    float metallic;
    float roughness;
    float ao;

    uint64_t albedoTexture;
    uint64_t normalTexture;
    //int roughnessTexture;
    int32_t pad0;
    int32_t pad1;
    //int32_t pad2;
    //int pad3;

    int32_t materialIndex;
};

struct Batch {
    uint32_t indexStart;
    uint32_t indexCount;
    int32_t material;
};

struct Mesh {
    int64_t positionOffset;
    int64_t normalOffset;
    int64_t tangentOffset;
    int64_t texCoord0Offset;
    //int32_t texCoord1Offset;
    int64_t indexOffset;
    uint32_t indexCount;
};

struct Model {
    uint64_t mesh;
    uint64_t material;
};

}