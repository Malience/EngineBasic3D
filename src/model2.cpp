#include "Model2.h"

#include "Util.h"
#include "ResourceSystem.h"
#include "pipeline.h"

namespace edl {

Model2::Model2() {
	position = {};
	rotation = {};
	scale = { 1.0f, 1.0f, 1.0f };

	model = glm::mat4(1);
	mvp = glm::mat4(1);
}

void Model2::draw(VkCommandBuffer cb, VkPipelineLayout layout) {
	//std::vector<uint32_t> dynamicOffsets({ 0 });
	//std::vector<VkDescriptorSet> sets = { uniforms_descriptor_handle.descriptorSet };
	//vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, sets.size(), sets.data(), dynamicOffsets.size(), dynamicOffsets.data());
	//drawMesh(cb, pipeline, *mesh);

}

void Model2::calculateModel() {
	glm::mat4 trans, rot, s;

	trans = glm::translate(glm::mat4(1), position);
	rot = glm::mat4_cast(rotation);
	s = glm::scale(glm::mat4(1), scale);
	
	model = trans * rot * s;
}

void Model2::update(ResourceSystem& system, const glm::mat4& proj, const glm::mat4& view) {
	mvp = proj * view * model;
	updateStorageBuffer(system.stagingBuffer, system.transformBuffer, transformHandle, &mvp, 1);
	//updateStorageBuffer(system.stagingBuffer, system.transformBuffer, transformHandle, &mvp, 1);
	//system.stagingBuffer.copyBufferToBuffer(&mvp, ubo_handle.buffer, ubo_handle.offset, ubo_handle.size);
}

}