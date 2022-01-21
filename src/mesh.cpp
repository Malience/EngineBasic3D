#include "Mesh.h"

#include "pipeline.h"
#include "Util.h"

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

void drawMesh(VkCommandBuffer cb, ShaderReflection& reflection, VkPipelineLayout layout, const Mesh2& mesh) {
    std::hash<std::string> hasher;

    for (int i = 0; i < reflection.attributes.size(); ++i) {
        std::string attributeName = reflection.attributes[i].name;
        for (int j = 0; j < mesh.attributeCount; j++) {
            if (hasher(attributeName) == mesh.attributes[j].nameHash) {
                uint32_t binding = reflection.attributes[i].location;
                VkDeviceSize offset = mesh.attributes[j].offset;
                vkCmdBindVertexBuffers(cb, binding, 1, &mesh.buffer, &offset); //TODO: This can be better
                break;
            }
        }
    }

    vkCmdBindIndexBuffer(cb, mesh.buffer, mesh.indexOffset, VK_INDEX_TYPE_UINT16);

    for (int i = 0; i < mesh.submeshCount; ++i) {
        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &mesh.submeshes[i].set, 0, nullptr);
        vkCmdDrawIndexed(cb, mesh.submeshes[i].indexCount, 1, mesh.submeshes[i].indexStart, 0, 0);
    }
}

}