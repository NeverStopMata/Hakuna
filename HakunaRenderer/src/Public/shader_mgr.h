#pragma once
#include <vulkan\vulkan.h>
#include "vulkan_utility.h"
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <iostream>
class ShaderManager
{
public:
	static ShaderManager* GetInstance()
	{
		if (p_instance_ == nullptr)
		{
			p_instance_ = new ShaderManager();
		}
		return p_instance_;
	}
	void SetShaderStages(string shader_name)
	{
		if (shader_stages_table_.end() == shader_stages_table_.find(shader_name))
		{
			vector<VkPipelineShaderStageCreateInfo> tmp_vector;
			shader_stages_table_[shader_name] = tmp_vector;
		}
		else
		{
			shader_stages_table_[shader_name].clear();
		}
		const string compiled_shader_path = "resource/shaders/compiled_shaders/";
		shader_stages_table_[shader_name].push_back(ShaderManager::GetInstance()->LoadShader(compiled_shader_path+ shader_name + "_vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
		shader_stages_table_[shader_name].push_back(ShaderManager::GetInstance()->LoadShader(compiled_shader_path+ shader_name + "_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
	}
	VkPipelineShaderStageCreateInfo LoadShader(std::string fileName, VkShaderStageFlagBits stage);
	std::vector<VkPipelineShaderStageCreateInfo>& GetShaderStagesByShaderName(std::string shader_name)
	{
		assert(shader_stages_table_.find(shader_name) != shader_stages_table_.end());
		{
			return shader_stages_table_[shader_name];
		}
	}
	void Init(VkDevice logical_device);
	void CleanShaderModules();
	~ShaderManager();
private:
	std::vector<char> ReadShaderFile(const std::string& filename);
	ShaderManager();
	VkShaderModule CreateShaderModule(const std::vector<char>& code);


	std::map<string, std::vector<VkPipelineShaderStageCreateInfo>> shader_stages_table_;
	VkDevice logical_device_;
	static ShaderManager* p_instance_;
	std::vector<VkShaderModule> shader_modules_;
};

