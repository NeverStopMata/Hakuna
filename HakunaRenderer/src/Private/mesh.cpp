#include "mesh.h"
void Mesh::CreateVertexBuffer(VulkanUtility::VulkanContex* vk_context_ptr) {
	std::vector<uint32_t> stadingBufferQueueFamilyIndices{ static_cast<uint32_t>(vk_context_ptr->queue_family_indices.transferFamily) };
	std::vector<uint32_t> deviceLocalBufferQueueFamilyIndices{
		static_cast<uint32_t>(vk_context_ptr->queue_family_indices.transferFamily),
		static_cast<uint32_t>(vk_context_ptr->queue_family_indices.graphicsFamily) };
	VkDeviceSize bufferSize = sizeof(this->vertices_[0]) * this->vertices_.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VulkanUtility::CreateBuffer(*vk_context_ptr, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory,
		1, nullptr);

	void* data;
	vkMapMemory(vk_context_ptr->logical_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, this->vertices_.data(), (size_t)bufferSize);
	vkUnmapMemory(vk_context_ptr->logical_device, stagingBufferMemory);

	VulkanUtility::CreateBuffer(*vk_context_ptr,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->vertex_buffer_,
		this->vertex_buffer_memory_,
		2,
		deviceLocalBufferQueueFamilyIndices.data());

	VulkanUtility::CopyBuffer(*vk_context_ptr, stagingBuffer, this->vertex_buffer_, bufferSize);

	vkDestroyBuffer(vk_context_ptr->logical_device, stagingBuffer, nullptr);
	vkFreeMemory(vk_context_ptr->logical_device, stagingBufferMemory, nullptr);
}

void Mesh::CreateIndexBuffer(VulkanUtility::VulkanContex* vk_context_ptr)
{
	std::vector<uint32_t> deviceLocalBufferQueueFamilyIndices{
		static_cast<uint32_t>(vk_context_ptr->queue_family_indices.transferFamily),
		static_cast<uint32_t>(vk_context_ptr->queue_family_indices.graphicsFamily) };
	VkDeviceSize bufferSize = sizeof(this->indices_[0]) * this->indices_.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VulkanUtility::CreateBuffer(*vk_context_ptr,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory,
		1,
		nullptr);

	void* data;
	vkMapMemory(vk_context_ptr->logical_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, this->indices_.data(), (size_t)bufferSize);
	vkUnmapMemory(vk_context_ptr->logical_device, stagingBufferMemory);

	VulkanUtility::CreateBuffer(*vk_context_ptr,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->index_buffer_,
		this->index_buffer_memory_,
		2,
		deviceLocalBufferQueueFamilyIndices.data());

	VulkanUtility::CopyBuffer(*vk_context_ptr, stagingBuffer, this->index_buffer_, bufferSize);

	vkDestroyBuffer(vk_context_ptr->logical_device, stagingBuffer, nullptr);
	vkFreeMemory(vk_context_ptr->logical_device, stagingBufferMemory, nullptr);
}