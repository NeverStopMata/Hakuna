#include "mesh.h"
#include "msoc_mgr.h"
void Mesh::GetAllTriangles(std::vector<Triangle>& triangles)
{
	triangles.clear();
	for (int i = 0; i < indices_.size(); i += 3)
	{
		triangles.emplace_back(Triangle{ array<glm::vec3,3>{vertices_[indices_[i]].pos,vertices_[indices_[i+1]].pos,vertices_[indices_[i+2]].pos}});
	}
}
void Mesh::CreateVertexBuffer(VulkanUtility::VulkanContex* vk_context_ptr) {
	std::vector<uint32_t> stadingBufferQueueFamilyIndices{ static_cast<uint32_t>(vk_context_ptr->vulkan_device.queue_family_indices.transferFamily) };
	std::vector<uint32_t> deviceLocalBufferQueueFamilyIndices{
		static_cast<uint32_t>(vk_context_ptr->vulkan_device.queue_family_indices.transferFamily),
		static_cast<uint32_t>(vk_context_ptr->vulkan_device.queue_family_indices.graphicsFamily) };
	VkDeviceSize bufferSize = sizeof(this->vertices_[0]) * this->vertices_.size();

	VulkanBuffer stagingBuffer;
	vk_context_ptr->vulkan_device.CreateBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		bufferSize
	);
	stagingBuffer.map(bufferSize);
	memcpy(stagingBuffer.mapped, this->vertices_.data(), (size_t)bufferSize);
	stagingBuffer.unmap();

	vk_context_ptr->vulkan_device.CreateBuffer(
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&this->vertex_buffer_,
		bufferSize,
		2,
		deviceLocalBufferQueueFamilyIndices.data()
	);

	vk_context_ptr->vulkan_device.CopyBuffer(stagingBuffer.buffer, this->vertex_buffer_.buffer, bufferSize);

	vkDestroyBuffer(vk_context_ptr->vulkan_device.logical_device, stagingBuffer.buffer, nullptr);
	vkFreeMemory(vk_context_ptr->vulkan_device.logical_device, stagingBuffer.memory, nullptr);
}

void Mesh::CreateIndexBuffer(VulkanUtility::VulkanContex* vk_context_ptr)
{
	std::vector<uint32_t> deviceLocalBufferQueueFamilyIndices{
		static_cast<uint32_t>(vk_context_ptr->vulkan_device.queue_family_indices.transferFamily),
		static_cast<uint32_t>(vk_context_ptr->vulkan_device.queue_family_indices.graphicsFamily) };
	VkDeviceSize bufferSize = sizeof(this->indices_[0]) * this->indices_.size();
	VulkanBuffer stagingBuffer;
	vk_context_ptr->vulkan_device.CreateBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		bufferSize);
	stagingBuffer.map(bufferSize);
	memcpy(stagingBuffer.mapped, this->indices_.data(), (size_t)bufferSize);
	stagingBuffer.unmap();
	
	vk_context_ptr->vulkan_device.CreateBuffer(
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&this->index_buffer_,
		bufferSize,
		2,
		deviceLocalBufferQueueFamilyIndices.data());
	vk_context_ptr->vulkan_device.CopyBuffer(stagingBuffer.buffer, this->index_buffer_.buffer, bufferSize);
	stagingBuffer.destroy();
}