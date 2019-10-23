#include "shader_manager.h"
void ShaderMgr::CleanShaderModules(const VulkanUtility::VulkanContex& vk_contex) {
	for (auto& shaderModule : shader_modules_)
	{
		vkDestroyShaderModule(vk_contex.vulkan_device.logical_device, shaderModule, nullptr);
	}
}
std::vector<char> ShaderMgr::ReadShaderFile(const std::string& filename) {
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

VkPipelineShaderStageCreateInfo ShaderMgr::LoadShader(const VulkanUtility::VulkanContex& vk_contex, std::string fileName, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	auto vertShaderCode = ShaderMgr::ReadShaderFile(fileName);
	VkShaderModule vertShaderModule = VulkanUtility::CreateShaderModule(vk_contex, vertShaderCode);
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
	shaderStage.module = vertShaderModule;
	shaderStage.pName = "main";
	shader_modules_.push_back(shaderStage.module);
	return shaderStage;
}
