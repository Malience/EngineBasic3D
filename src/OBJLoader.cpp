#include "ResourceSystem.h"

#include "edl/logger.h"
#include "mesh.h"

#include "rapidobj/rapidobj.hpp"

#include <unordered_set>
#include <iostream>

namespace edl {

void loadOBJ(Toolchain& toolchain, Resource& res) {
    edl::ResourceSystem& system = toolchain.getTool<edl::ResourceSystem>("system");

    edl::allocateResourceData(res, sizeof(Mesh), *system.allocator);
    Mesh& meshres = edl::getResourceData<Mesh>(res);

    auto result = rapidobj::ParseFile(res.path);

    if (result.error) {
        std::cout << result.error.code.message() << std::endl;
        return;
    }

    /* Triangulation is broken
    if (!rapidobj::Triangulate(result)) {
        std::cout << result.error.code.message() << std::endl;
    }
    */

    for (auto& shape : result.shapes) {
        for (auto& face : shape.mesh.num_face_vertices) {
            assert(face == 3);
        }
    }

    const uint32_t SIZEOF_MAX_TRIANGLES = 20;
    const uint32_t SIZEOF_MAX_VERTICES = SIZEOF_MAX_TRIANGLES * 3;

    uint32_t total = 0;

    for (auto& shape : result.shapes) {
        total += shape.mesh.num_face_vertices.size();
    }

    uint32_t meshletCount = (total / SIZEOF_MAX_TRIANGLES) + ((total % SIZEOF_MAX_TRIANGLES) > 0);

    MeshHeader mesh = {};
    std::vector<MeshletHeader> meshlets(meshletCount);
    MeshletData data;
    data.indices.resize(total * 3);
    data.materials.resize(total);

    data.positions.resize(total * 3);
    data.normals.resize(total * 3);
    data.tangents.resize(total * 3);
    data.texcoords.resize(total * 3);

    uint32_t idx = 0;

    for (auto& shape : result.shapes) {
        size_t faceCount = shape.mesh.num_face_vertices.size();

        for (uint32_t f = 0; f < faceCount; f++) {

            data.materials[idx] = shape.mesh.material_ids[f];

            for (uint32_t i = 0; i < 3; i++) {
                const auto& oidx = shape.mesh.indices[f * 3 + i];

                // Create a new vertex
                const auto& posx = result.attributes.positions[oidx.position_index * 3 + 0];
                const auto& posy = result.attributes.positions[oidx.position_index * 3 + 1];
                const auto& posz = result.attributes.positions[oidx.position_index * 3 + 2];
                data.positions[idx * 3 + i] = glm::vec4(posx, posy, posz, 1.0f);

                const auto& normx = result.attributes.normals[oidx.normal_index * 3 + 0];
                const auto& normy = result.attributes.normals[oidx.normal_index * 3 + 1];
                const auto& normz = result.attributes.normals[oidx.normal_index * 3 + 2];
                data.normals[idx * 3 + i] = glm::vec4(normx, normy, normz, 0.0f);

                const auto& texx = result.attributes.texcoords[oidx.texcoord_index * 2 + 0];
                const auto& texy = result.attributes.texcoords[oidx.texcoord_index * 2 + 1];
                data.texcoords[idx * 3 + i] = glm::vec2(texx, texy);

                data.indices[idx * 3 + i] = (idx * 3 + i) % SIZEOF_MAX_VERTICES;
            }

            glm::vec4 v0 = data.positions[idx * 3 + 0];
            glm::vec4 v1 = data.positions[idx * 3 + 1];
            glm::vec4 v2 = data.positions[idx * 3 + 2];

            glm::vec2 uv0 = data.texcoords[idx * 3 + 0];
            glm::vec2 uv1 = data.texcoords[idx * 3 + 1];
            glm::vec2 uv2 = data.texcoords[idx * 3 + 2];

            glm::vec4 e10 = v1 - v0;
            glm::vec4 e20 = v2 - v0;
            glm::vec2 duv10 = uv1 - uv0;
            glm::vec2 duv20 = uv2 - uv0;

            float denom = 1.0f / (duv10.x * duv20.y - duv20.x * duv10.y);

            glm::vec4 tangent = glm::vec4(
                denom * (duv20.y * e10.x - duv10.y * e20.x),
                denom * (duv20.y * e10.y - duv10.y * e20.y),
                denom * (duv20.y * e10.z - duv10.y * e20.z),
                0.0f
            );

            data.tangents[idx * 3 + 0] = tangent;
            data.tangents[idx * 3 + 1] = tangent;
            data.tangents[idx * 3 + 2] = tangent;

            idx++;
        }
    }

    int meshIndex = edl::getStorageBufferIndex(system.geometryBuffer, sizeof(MeshHeader), 0x10);
    meshres.positionOffset = edl::getStorageBufferAddress(system.geometryBuffer, meshIndex);
    int meshletsIndex = edl::getStorageBufferIndex(system.geometryBuffer, sizeof(MeshletHeader) * meshletCount, 0x10);

    mesh.meshletCount = meshletCount;
    mesh.meshletsAddress = getStorageBufferAddress(system.geometryBuffer, meshletsIndex);;
    mesh.padding0 = 0;

    edl::updateStorageBuffer(system.stagingBuffer, system.geometryBuffer, meshIndex, &mesh, sizeof(MeshHeader));

    // TODO: HACK, THIS MUST BE ReMOVED
    meshres.indexCount = meshletCount;
    meshres.indexOffset = total;

    for (uint32_t i = 0; i < meshletCount; i++) {
        uint32_t tricount = (i == meshletCount - 1) ? total % SIZEOF_MAX_TRIANGLES : SIZEOF_MAX_TRIANGLES;
        uint32_t vertcount = tricount * 3;

        MeshletHeader& meshlet = meshlets[i];
        meshlet.triCount = tricount;
        meshlet.vertCount = vertcount;

        uint32_t positionsIndex = edl::getStorageBufferIndex(system.geometryBuffer, sizeof(glm::vec4) * meshlet.vertCount, 0);
        uint32_t normalsIndex = edl::getStorageBufferIndex(system.geometryBuffer, sizeof(glm::vec4) * meshlet.vertCount, 0x10);
        uint32_t tangentsIndex = edl::getStorageBufferIndex(system.geometryBuffer, sizeof(glm::vec4) * meshlet.vertCount, 0x10);
        uint32_t texcoordsIndex = edl::getStorageBufferIndex(system.geometryBuffer, sizeof(glm::vec2) * meshlet.vertCount, 0x10);

        meshlet.positionsAddress = edl::getStorageBufferAddress(system.geometryBuffer, positionsIndex);
        meshlet.normalsAddress = edl::getStorageBufferAddress(system.geometryBuffer, normalsIndex);
        meshlet.tangentsAddress = edl::getStorageBufferAddress(system.geometryBuffer, tangentsIndex);
        meshlet.texcoordsAddress = edl::getStorageBufferAddress(system.geometryBuffer, texcoordsIndex);

        uint32_t indicesIndex = edl::getStorageBufferIndex(system.geometryBuffer, sizeof(uint32_t) * meshlet.triCount * 3, 0x10);
        uint32_t materialsIndex = edl::getStorageBufferIndex(system.geometryBuffer, sizeof(uint32_t) * meshlet.triCount, 0x10);

        meshlet.indicesAddress = edl::getStorageBufferAddress(system.geometryBuffer, indicesIndex);
        meshlet.materialsAddress = edl::getStorageBufferAddress(system.geometryBuffer, materialsIndex);

        edl::updateStorageBuffer(system.stagingBuffer, system.geometryBuffer, positionsIndex, &data.positions[i * SIZEOF_MAX_VERTICES], sizeof(glm::vec4) * meshlet.vertCount);
        edl::updateStorageBuffer(system.stagingBuffer, system.geometryBuffer, normalsIndex, &data.normals[i * SIZEOF_MAX_VERTICES], sizeof(glm::vec4) * meshlet.vertCount);
        edl::updateStorageBuffer(system.stagingBuffer, system.geometryBuffer, tangentsIndex, &data.tangents[i * SIZEOF_MAX_VERTICES], sizeof(glm::vec4) * meshlet.vertCount);
        edl::updateStorageBuffer(system.stagingBuffer, system.geometryBuffer, texcoordsIndex, &data.texcoords[i * SIZEOF_MAX_VERTICES], sizeof(glm::vec2) * meshlet.vertCount);

        edl::updateStorageBuffer(system.stagingBuffer, system.geometryBuffer, indicesIndex, &data.indices[i * SIZEOF_MAX_TRIANGLES * 3], sizeof(uint32_t) * meshlet.triCount * 3);
        edl::updateStorageBuffer(system.stagingBuffer, system.geometryBuffer, materialsIndex, &data.materials[i * SIZEOF_MAX_TRIANGLES], sizeof(uint32_t) * meshlet.triCount);
    }

    edl::updateStorageBuffer(system.stagingBuffer, system.geometryBuffer, meshletsIndex, meshlets.data(), sizeof(MeshletHeader) * meshlets.size());
    
    res.status = edl::ResourceStatus::LOADED;
}

}