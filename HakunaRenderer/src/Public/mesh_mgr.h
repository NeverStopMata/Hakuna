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
public:
	~MeshMgr();
	shared_ptr<Mesh> GetMeshByName(string mesh_name);
	static MeshMgr* GetInstance()
	{
		if (p_instance_ == nullptr)
		{
			p_instance_ = new MeshMgr();
		}
		return p_instance_;
	}
	void Init(VulkanUtility::VulkanContex* vk_context);
	void LoadModelFromFile(std::string model_path, std::string name, glm::vec3 scale = glm::vec3(1,1,1));
private:
	static MeshMgr* p_instance_;
	MeshMgr();
	void CalculateTangents(std::array<Vertex, 3>& vertices);
	void CreateCubeMesh(std::string name, glm::vec3 scale);
	void CleanUpMeshDict();


	VulkanUtility::VulkanContex* vk_context_ptr_;
	std::map<std::string, std::shared_ptr<Mesh>> mesh_dict_;
};

