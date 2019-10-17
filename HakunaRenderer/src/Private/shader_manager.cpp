#include "shader_manager.h"



ShaderManager::ShaderManager()
{
}


ShaderManager::~ShaderManager()
{
}
void ShaderManager::CleanShaderModules(const VulkanUtility::VulkanContex& contex) {
	for (auto& shaderModule : shader_modules_)
	{
		vkDestroyShaderModule(contex.logical_device, shaderModule, nullptr);
	}
}
std::vector<char> ShaderManager::ReadShaderFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

VkPipelineShaderStageCreateInfo ShaderManager::LoadShader(const VulkanUtility::VulkanContex& vk_contex, std::string fileName, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	auto vertShaderCode = ShaderManager::ReadShaderFile(fileName);
	VkShaderModule vertShaderModule = VulkanUtility::CreateShaderModule(vk_contex, vertShaderCode);
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
	shaderStage.module = vertShaderModule;
	shaderStage.pName = "main";
	shader_modules_.push_back(shaderStage.module);
	return shaderStage;
}
