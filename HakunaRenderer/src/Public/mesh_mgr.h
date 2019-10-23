#pragma once
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <vector>
#include <vulkan\vulkan.h>
#include <array>
#include <unordered_map>
#include <map>
#include "vulkan_utility.h"
#include "mesh.h"
#include "vertex.h"
#include <iostream>
using namespace std;

class MeshMgr
{

private:
	MeshMgr() {}
	~MeshMgr() {}
	MeshMgr(const MeshMgr&);
	MeshMgr& operator=(const MeshMgr&);
	std::map<std::string, std::shared_ptr<Mesh>> mesh_dict_;
	VulkanUtility::VulkanContex* vk_context_ptr_;
public:
	static MeshMgr& GetInstance();
	//std::vector<Vertex> vertices_;
	//std::vector<uint32_t> indices_;

	void Init(VulkanUtility::VulkanContex* vk_context);
	void CalculateTangents(std::array<Vertex, 3>& vertices);
	void CreateCubeMesh(std::string name, glm::vec3 scale);
	void LoadModelFromFile(std::string model_path, std::string name, glm::vec3 scale = glm::vec3(1,1,1));
	void CleanUpMeshDict();
	//void CreateVertexBuffer(Mesh& mesh);
	//void CreateIndexBuffer(Mesh& mesh);
	shared_ptr<Mesh> GetMeshByName(string mesh_name);
};

