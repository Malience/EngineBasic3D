#pragma once

#include <stdint.h>

const uint32_t MESHLET_MAX_VERTICES = 64;
const uint32_t MESHLET_MAX_TRIANGLES = 126;

#pragma pack(push, 1)
struct MeshHeader {
    uint32_t meshletCount;
    uint32_t padding0;
    uint64_t meshletsAddress;
};

struct MeshletHeader {
    uint32_t vertCount;
    uint32_t triCount;

    uint64_t positionsAddress;
    uint64_t normalsAddress;
    uint64_t tangentsAddress;
    uint64_t texcoordsAddress;

    uint64_t indicesAddress;
    uint64_t materialsAddress;

    uint64_t padding0;
};

struct MeshletData {
    // Per-Vertex
    std::vector<glm::vec4> positions;
    std::vector<glm::vec4> normals;
    std::vector<glm::vec4> tangents;
    std::vector<glm::vec2> texcoords;

    // Per-Face
    std::vector<uint32_t> indices; // TODO: I hate that this isn't ushort
    std::vector<uint32_t> materials; // This too
};

struct Mesh2 {
    MeshHeader header;
    std::vector<MeshletHeader> meshlets;
    std::vector<MeshletData> data;
};
#pragma pack(pop)