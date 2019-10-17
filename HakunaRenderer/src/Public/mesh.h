#pragma once
#include <vulkan\vulkan.h>
#include <vector>
#include <glm/glm.hpp>
#include "vulkan_utility.h"

class Mesh
{
	friend class MeshMgr;
public:
	std::vector<Vertex> vertices_;
	std::vector<uint32_t> indices_;
	VkBuffer vertex_buffer_ = VK_NULL_HANDLE;
	VkDeviceMemory vertex_buffer_memory_ = VK_NULL_HANDLE;
	VkBuffer index_buffer_ = VK_NULL_HANDLE;
	VkDeviceMemory index_buffer_memory_ = VK_NULL_HANDLE;
private:
	void CreateVertexBuffer(VulkanUtility::VulkanContex* vk_context_ptr);
	void CreateIndexBuffer(VulkanUtility::VulkanContex* vk_context_ptr);
};

