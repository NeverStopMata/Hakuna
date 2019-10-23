#pragma once
#include <vulkan\vulkan.h>
#include "vulkan_utility.h"
#include <vector>
#include <string>
#include <fstream>
class ShaderMgr
{
public:
	static ShaderMgr& GetInstance()
	{
		static ShaderMgr instance;
		return instance;
	}

	VkPipelineShaderStageCreateInfo LoadShader(const VulkanUtility::VulkanContex& vk_contex, std::string fileName, VkShaderStageFlagBits stage);
	void CleanShaderModules(const VulkanUtility::VulkanContex& contex);
private:
	std::vector<char> ReadShaderFile(const std::string& filename);

private:
	ShaderMgr() {};
	~ShaderMgr() {};
	ShaderMgr(const ShaderMgr&);
	ShaderMgr& operator=(const ShaderMgr&);
	std::vector<VkShaderModule> shader_modules_;
};

