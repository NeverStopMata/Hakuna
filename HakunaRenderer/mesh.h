#pragma once
#include <vulkan\vulkan.h>
#include <vector>
#include <glm/glm.hpp>
#include "vulkan_utility.h"




class Mesh
{
public:
	Mesh();
	~Mesh();

public:
	std::vector<Vertex> vertices_;
	std::vector<uint32_t> indices_;
	VkBuffer vertex_buffer_;
	VkDeviceMemory vertex_buffer_memory_;
	VkBuffer index_buffer_;
	VkDeviceMemory index_buffer_memory_;
};

