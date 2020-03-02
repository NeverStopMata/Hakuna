#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include "mesh_mgr.h"
MeshMgr* MeshMgr::p_instance_ = nullptr;
MeshMgr::MeshMgr() {}
MeshMgr::~MeshMgr()
{
	CleanUpMeshDict();
}


void MeshMgr::Init(VulkanUtility::VulkanContex* vk_context) {
	vk_context_ptr_ = vk_context;
}

void MeshMgr::LoadModelFromFile(std::string model_path,std::string name, glm::vec3 scale) {
	if (mesh_dict_.find(name) != mesh_dict_.end()){
		return;
	}
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str())) {
		throw std::runtime_error(warn + err);
	}
	auto new_mesh_ptr_ = std::make_shared<Mesh>();
	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
	for (const auto& shape : shapes) {
		//const auto& index : shape.mesh.indices
		for (int i = 0; i < shape.mesh.indices.size();i+=3) {
			std::array<Vertex, 3> tmp_vertices;
			for (int j = i; j < i + 3; j++) {
				tmp_vertices[j - i].pos = {
					attrib.vertices[3 * shape.mesh.indices[j].vertex_index + 0] * scale.x,
					attrib.vertices[3 * shape.mesh.indices[j].vertex_index + 1] * scale.y,
					attrib.vertices[3 * shape.mesh.indices[j].vertex_index + 2] * scale.z
				};
				tmp_vertices[j - i].normal = {
					attrib.normals[3 * shape.mesh.indices[j].normal_index + 0],
					attrib.normals[3 * shape.mesh.indices[j].normal_index + 1],
					attrib.normals[3 * shape.mesh.indices[j].normal_index + 2]
				};
				if (attrib.texcoords.size() != 0) {
					tmp_vertices[j - i].texCoord = {
						attrib.texcoords[2 * shape.mesh.indices[j].texcoord_index + 0],
						1.0f - attrib.texcoords[2 * shape.mesh.indices[j].texcoord_index + 1]
					};
				}
				else {
					tmp_vertices[j - i].texCoord = {0,0};
				}
				tmp_vertices[j - i].color = { 1.0f, 1.0f, 1.0f };
			}
			if (attrib.texcoords.size() != 0)
				CalculateTangents(tmp_vertices);
			for (int k = 0; k < 3; k++) {
				if (uniqueVertices.count(tmp_vertices[k]) == 0) {
					uniqueVertices[tmp_vertices[k]] = static_cast<uint32_t>(new_mesh_ptr_->vertices_.size());
					new_mesh_ptr_->AddNewVertex(tmp_vertices[k]);
				}
				new_mesh_ptr_->indices_.push_back(uniqueVertices[tmp_vertices[k]]);
			}
		}
	}
	new_mesh_ptr_->CreateVertexBuffer(vk_context_ptr_);
	new_mesh_ptr_->CreateIndexBuffer(vk_context_ptr_);
	mesh_dict_[name] = new_mesh_ptr_;
}

void MeshMgr::CreateCubeMesh(std::string name, glm::vec3 scale) {
	auto new_mesh_ptr_ = std::make_shared<Mesh>();
	new_mesh_ptr_->vertices_.resize(24);
	for (int faceIdx = 0; faceIdx < 6; faceIdx++)
	{
		glm::vec3 faceNormal = glm::vec3(((faceIdx % 2) * 2 - 1) * (faceIdx / 2 == 0), ((faceIdx % 2) * 2 - 1)*(-1) * (faceIdx / 2 == 1), ((faceIdx % 2) * 2 - 1) * (faceIdx / 2 == 2));
		for (int i = 0; i < 4; i++)
		{
			new_mesh_ptr_->vertices_[faceIdx * 4 + i].pos = faceNormal
			+glm::vec3(0, (i % 2) * 2 - 1, (i / 2) * 2 - 1) * abs(faceNormal.x)
			+glm::vec3((i % 2) * 2 - 1, 0, (i / 2) * 2 - 1) * abs(faceNormal.y)
			+glm::vec3((i % 2) * 2 - 1, (i / 2) * 2 - 1, 0) * abs(faceNormal.z);
			new_mesh_ptr_->vertices_[faceIdx * 4 + i].normal = faceNormal;
			new_mesh_ptr_->vertices_[faceIdx * 4 + i].pos *= scale;
		}
	}
	new_mesh_ptr_->indices_.resize(36);
	int idxCnt = 0;
	for (int faceIdx = 0; faceIdx < 6; faceIdx++)
	{	
		if (faceIdx % 2 == 0)
		{
			new_mesh_ptr_->indices_[idxCnt++] = 2 + faceIdx * 4;
			new_mesh_ptr_->indices_[idxCnt++] = 1 + faceIdx * 4;
			new_mesh_ptr_->indices_[idxCnt++] = 0 + faceIdx * 4;
			new_mesh_ptr_->indices_[idxCnt++] = 1 + faceIdx * 4;
			new_mesh_ptr_->indices_[idxCnt++] = 2 + faceIdx * 4;
			new_mesh_ptr_->indices_[idxCnt++] = 3 + faceIdx * 4;
		}
		else
		{
			new_mesh_ptr_->indices_[idxCnt++] = 0 + faceIdx * 4;
			new_mesh_ptr_->indices_[idxCnt++] = 1 + faceIdx * 4;
			new_mesh_ptr_->indices_[idxCnt++] = 2 + faceIdx * 4;
			new_mesh_ptr_->indices_[idxCnt++] = 2 + faceIdx * 4;
			new_mesh_ptr_->indices_[idxCnt++] = 1 + faceIdx * 4;
			new_mesh_ptr_->indices_[idxCnt++] = 3 + faceIdx * 4;
		}

	}
	new_mesh_ptr_->CreateVertexBuffer(vk_context_ptr_);
	new_mesh_ptr_->CreateIndexBuffer(vk_context_ptr_);
	mesh_dict_[name] = new_mesh_ptr_;
}


void MeshMgr::CalculateTangents(std::array<Vertex, 3>& vertices) {
	for (int i = 0; i < 3; i++) {
		glm::vec2 uv_v1 = vertices[(i + 1) % 3].texCoord - vertices[i].texCoord;
		glm::vec2 uv_v2 = vertices[(i + 2) % 3].texCoord - vertices[i].texCoord;

		glm::vec3 pos_v1 = vertices[(i + 1) % 3].pos - vertices[i].pos;
		glm::vec3 pos_v2 = vertices[(i + 2) % 3].pos - vertices[i].pos;

		float f = 1.0f / (uv_v1.x * uv_v2.y - uv_v2.x * uv_v1.y);
		float u = uv_v2.y * f;
		float v = -uv_v1.y * f;

		vertices[i].tangent   = glm::normalize(u * pos_v1 + v * pos_v2);
		vertices[i].bitangent = glm::cross(vertices[i].tangent, vertices[i].normal);
	}
}

void MeshMgr::CleanUpMeshDict() {
	for (auto kv : mesh_dict_) {
		kv.second->vertex_buffer_.destroy();
		kv.second->index_buffer_.destroy();
		kv.second->vertices_.clear();
		kv.second->vertices_.shrink_to_fit();
		kv.second->indices_.clear();
		kv.second->indices_.shrink_to_fit();
	}
}
//
//void MeshMgr::CreateVertexBuffer(Mesh& mesh) {
//	std::vector<uint32_t> stadingBufferQueueFamilyIndices{ static_cast<uint32_t>(vk_context_ptr_->queue_family_indices.transferFamily) };
//	std::vector<uint32_t> deviceLocalBufferQueueFamilyIndices{
//		static_cast<uint32_t>(vk_context_ptr_->queue_family_indices.transferFamily),
//		static_cast<uint32_t>(vk_context_ptr_->queue_family_indices.graphicsFamily) };
//	VkDeviceSize bufferSize = sizeof(mesh.vertices_[0]) * mesh.vertices_.size();
//
//	VkBuffer stagingBuffer;
//	VkDeviceMemory stagingBufferMemory;
//	VulkanUtility::CreateBuffer(*vk_context_ptr_, bufferSize,
//		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//		stagingBuffer,
//		stagingBufferMemory,
//		1, nullptr);
//
//	void* data;
//	vkMapMemory(vk_context_ptr_->logical_device, stagingBufferMemory, 0, bufferSize, 0, &data);
//	memcpy(data, mesh.vertices_.data(), (size_t)bufferSize);
//	vkUnmapMemory(vk_context_ptr_->logical_device, stagingBufferMemory);
//
//	VulkanUtility::CreateBuffer(*vk_context_ptr_,
//		bufferSize,
//		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
//		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
//		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//		mesh.vertex_buffer_,
//		mesh.vertex_buffer_memory_,
//		2,
//		deviceLocalBufferQueueFamilyIndices.data());
//
//	VulkanUtility::CopyBuffer(*vk_context_ptr_, stagingBuffer, mesh.vertex_buffer_, bufferSize);
//
//	vkDestroyBuffer(vk_context_ptr_->logical_device, stagingBuffer, nullptr);
//	vkFreeMemory(vk_context_ptr_->logical_device, stagingBufferMemory, nullptr);
//}
//
//void MeshMgr::CreateIndexBuffer(Mesh& mesh)
//{
//	std::vector<uint32_t> deviceLocalBufferQueueFamilyIndices{
//		static_cast<uint32_t>(vk_context_ptr_->queue_family_indices.transferFamily),
//		static_cast<uint32_t>(vk_context_ptr_->queue_family_indices.graphicsFamily) };
//	VkDeviceSize bufferSize = sizeof(mesh.indices_[0]) * mesh.indices_.size();
//
//	VkBuffer stagingBuffer;
//	VkDeviceMemory stagingBufferMemory;
//	VulkanUtility::CreateBuffer(*vk_context_ptr_,
//		bufferSize,
//		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//		stagingBuffer,
//		stagingBufferMemory,
//		1,
//		nullptr);
//
//	void* data;
//	vkMapMemory(vk_context_ptr_->logical_device, stagingBufferMemory, 0, bufferSize, 0, &data);
//	memcpy(data, mesh.indices_.data(), (size_t)bufferSize);
//	vkUnmapMemory(vk_context_ptr_->logical_device, stagingBufferMemory);
//
//	VulkanUtility::CreateBuffer(*vk_context_ptr_,
//		bufferSize,
//		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
//		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
//		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//		mesh.index_buffer_,
//		mesh.index_buffer_memory_,
//		2,
//		deviceLocalBufferQueueFamilyIndices.data());
//
//	VulkanUtility::CopyBuffer(*vk_context_ptr_, stagingBuffer, mesh.index_buffer_, bufferSize);
//
//	vkDestroyBuffer(vk_context_ptr_->logical_device, stagingBuffer, nullptr);
//	vkFreeMemory(vk_context_ptr_->logical_device, stagingBufferMemory, nullptr);
//}

shared_ptr<Mesh> MeshMgr::GetMeshByName(string mesh_name) {
	return mesh_dict_[mesh_name];
}