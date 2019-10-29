#pragma once
#include <vulkan\vulkan.h>
#include "vulkan_utility.h"
#include <vector>
#include <string>
#include <fstream>
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
	VkPipelineShaderStageCreateInfo LoadShader(std::string fileName, VkShaderStageFlagBits stage);
	void Init(VkDevice logical_device);
	void CleanShaderModules();
	~ShaderManager();
private:
	std::vector<char> ReadShaderFile(const std::string& filename);
	ShaderManager();
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	VkDevice logical_device_;
	static ShaderManager* p_instance_;
	std::vector<VkShaderModule> shader_modules_;
};

