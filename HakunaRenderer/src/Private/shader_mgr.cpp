#include "shader_mgr.h"


ShaderManager* ShaderManager::p_instance_ = nullptr;
ShaderManager::ShaderManager()
{
}

VkShaderModule ShaderManager::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(logical_device_, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
}


ShaderManager::~ShaderManager()
{
	CleanShaderModules();
}
void ShaderManager::CleanShaderModules()
{
	for (auto& shaderModule : shader_modules_)
	{
		vkDestroyShaderModule(logical_device_, shaderModule, nullptr);
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

VkPipelineShaderStageCreateInfo ShaderManager::LoadShader(std::string fileName, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	auto vertShaderCode = ShaderManager::ReadShaderFile(fileName);
	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
	shaderStage.module = vertShaderModule;
	shaderStage.pName = "main";
	shader_modules_.push_back(shaderStage.module);
	return shaderStage;
}

void ShaderManager::Init(VkDevice logical_device)
{
	logical_device_ = logical_device;
}
