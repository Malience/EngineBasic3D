#pragma once

#include "image_table.h"
#include "StagingBuffer.h"
#include "descriptor_manager.h"
#include "StorageBuffer.h"
#include "edl/vk/vulkan.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "ResourceHandles.h"
#include "Mesh.h"

#include <vector>
#include <string>

namespace edl {

class ResourceSystem;
class Pipeline;

class Model2 {
public:
    Model2();
    void draw(VkCommandBuffer cb, VkPipelineLayout layout);
    void calculateModel();
    void update(ResourceSystem& system, const glm::mat4& proj, const glm::mat4& view);

    std::string name;

    std::string pipeline;

    std::vector<ImageHandle> textures;

    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    glm::mat4 model;
    glm::mat4 mvp;

    uint32_t transformHandle;
    ResourceID mesh;
    //DescriptorHandle uniforms_descriptor_handle;
};

}