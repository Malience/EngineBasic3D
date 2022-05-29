#include "Mesh2.h"

#include "edl/util.h"

namespace edl {

size_t callocMesh(void* memory, uint32_t attributeCount, uint32_t submeshCount) {
    size_t offset = 0;

    Mesh2* mesh = static_cast<Mesh2*>(memory);
    mesh->buffer = 0;
    mesh->indexCount = 0;
    mesh->indexOffset = 0;

    offset += sizeof(Mesh2);

    mesh->attributeCount = attributeCount;
    mesh->attributes = edl::getPointerOffset<Attribute>(memory, offset);

    offset += attributeCount * sizeof(Attribute);

    mesh->submeshCount = submeshCount;
    mesh->submeshes = edl::getPointerOffset<SubMesh>(memory, offset);

    offset += submeshCount * sizeof(SubMesh);

    for (uint32_t i = 0; i < attributeCount; i++) {
        Attribute& attribute = mesh->attributes[i];
        attribute.offset = 0;
        attribute.nameHash = 0;
    }

    for (uint32_t i = 0; i < submeshCount; i++) {
        SubMesh& submesh = mesh->submeshes[i];
        submesh.indexCount = 0;
        submesh.indexStart = 0;
        submesh.set = 0;
    }

    return offset;
}

}