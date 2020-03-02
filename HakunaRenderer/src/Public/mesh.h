#pragma once
#include <vulkan\vulkan.h>
#include <vector>
#include <glm/glm.hpp>
#include "vulkan_utility.h"
#include "VulkanBuffer.hpp"
#include "bbox.h"
class Triangle;
class Mesh
{
	friend class MeshMgr;
public:
	Mesh()
	{
		ptr_bbox_ = make_shared<Bbox>();
	}
	std::vector<Vertex> vertices_;
	std::vector<uint32_t> indices_;
	VulkanBuffer vertex_buffer_;
	VulkanBuffer index_buffer_;
	std::shared_ptr<Bbox> ptr_bbox_;
	void AddNewVertex(Vertex new_vertex)
	{
		vertices_.push_back(new_vertex);
		UpdateBoundingBox(new_vertex.pos);
	}
	void GetAllTriangles(std::vector<Triangle>& triangles);

private:
	void CreateVertexBuffer(VulkanUtility::VulkanContex* vk_context_ptr);
	void CreateIndexBuffer(VulkanUtility::VulkanContex* vk_context_ptr);
	void UpdateBoundingBox(glm::vec3 new_vertex_position)
	{
		this->ptr_bbox_->max_ = glm::max(this->ptr_bbox_->max_, new_vertex_position);
		this->ptr_bbox_->min_ = glm::min(this->ptr_bbox_->min_, new_vertex_position);
	}

};

