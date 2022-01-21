#include "MeshLoader.h"

#include "ResourceSystem.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <iostream>

namespace edl {

void loadMesh(res::Toolchain& toolchain, res::Resource& res) {
    edl::ResourceSystem& system = toolchain.getTool<edl::ResourceSystem>("system");

    edl::res::allocateResourceData(res, sizeof(Mesh), *system.allocator);
    Mesh& meshres = edl::res::getResourceData<Mesh>(res);

    Assimp::Importer imp;
    const aiScene* scene = imp.ReadFile(res.path,
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType);

    if (scene == nullptr) {
        std::cout << imp.GetErrorString() << std::endl;
        return;
    }

    // Load the first mesh, don't load more >.>
    auto& mesh = scene->mMeshes[0];

    uint32_t vertexCount = mesh->mNumVertices;

    meshres.positionOffset = edl::getStorageBufferIndex(system.positionBuffer, vertexCount);
    meshres.normalOffset = edl::getStorageBufferIndex(system.normalBuffer, vertexCount);
    meshres.tangentOffset = edl::getStorageBufferIndex(system.normalBuffer, vertexCount);
    //uint32_t bitangentOffset = edl::getStorageBufferIndex(system.normalBuffer, vertexCount);
    meshres.texCoord0Offset = edl::getStorageBufferIndex(system.texCoord0Buffer, vertexCount);

    meshres.indexOffset = edl::getStorageBufferIndex(system.indexBuffer, mesh->mFaces->mNumIndices);

    std::vector<glm::vec4> positions(vertexCount);
    std::vector<glm::vec4> normals(vertexCount);
    std::vector<glm::vec4> tangents(vertexCount);
    //std::vector<glm::vec4> bitangents(vertexCount);

    std::vector<glm::vec2> texCoords0(vertexCount);

    for (int i = 0; i < vertexCount; i++) {
        auto& v = mesh->mVertices[i];
        auto& n = mesh->mNormals[i];
        auto& nt = mesh->mTangents[i];
        auto& nb = mesh->mBitangents[i];
        auto& tv = mesh->mTextureCoords[0][i];

        positions[i].x = v.x;
        positions[i].y = v.y;
        positions[i].z = v.z;
        positions[i].w = 1.0f;

        normals[i].x = n.x;
        normals[i].y = n.y;
        normals[i].z = n.z;
        normals[i].w = 0.0f;

        tangents[i].x = nt.x;
        tangents[i].y = nt.y;
        tangents[i].z = nt.z;
        tangents[i].w = 0.0f;

        //bitangents[i].x = nb.x;
        //bitangents[i].y = nb.y;
        //bitangents[i].z = nb.z;
        //bitangents[i].w = 0.0f;

        // TexCoords

        texCoords0[i].x = tv.x;
        texCoords0[i].y = tv.y;
    }

    std::vector<unsigned short> indices(mesh->mNumFaces * 3);
    for (int i = 0; i < mesh->mNumFaces; i++) {
        indices[i * 3 + 0] = mesh->mFaces[i].mIndices[0];
        indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
        indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
    }

    meshres.indexCount = mesh->mNumFaces * 3;

    edl::updateStorageBuffer(system.stagingBuffer, system.positionBuffer, meshres.positionOffset, positions.data(), vertexCount);
    edl::updateStorageBuffer(system.stagingBuffer, system.normalBuffer, meshres.normalOffset, normals.data(), vertexCount);
    edl::updateStorageBuffer(system.stagingBuffer, system.normalBuffer, meshres.tangentOffset, tangents.data(), vertexCount);
    //edl::updateStorageBuffer(system.stagingBuffer, system.normalBuffer, bitangentOffset, bitangents.data(), vertexCount);
    edl::updateStorageBuffer(system.stagingBuffer, system.texCoord0Buffer, meshres.texCoord0Offset, texCoords0.data(), vertexCount);

    edl::updateStorageBuffer(system.stagingBuffer, system.indexBuffer, meshres.indexOffset, indices.data(), mesh->mNumFaces * 3);

    res.status = edl::res::ResourceStatus::LOADED;
}

}