#pragma once
#include <vulkan\vulkan.h>
#include <vector>
#include <glm/glm.hpp>
#include "vulkan_utility.h"
#include "VulkanBuffer.hpp"
class Mesh
{
	friend class MeshMgr;
public:
	std::vector<Vertex> vertices_;
	std::vector<uint32_t> indices_;
	VulkanBuffer vertex_buffer_;
	VulkanBuffer index_buffer_;
private:
	void CreateVertexBuffer(VulkanUtility::VulkanContex* vk_context_ptr);
	void CreateIndexBuffer(VulkanUtility::VulkanContex* vk_context_ptr);
};

