#include "texture_mgr.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

TextureMgr::TextureMgr()
{
}

TextureMgr::~TextureMgr()
{
}

void TextureMgr::CreateTextureImage(const VulkanUtility::VulkanContex& vk_contex,string file_path, string tex_name) {
	tex_dict[tex_name] = make_shared<Texture>();
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(file_path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	uint32_t miplevel_size = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	tex_dict[tex_name]->miplevel_size = miplevel_size;
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (pixels == NULL) {
		throw std::runtime_error("failed to load texture image!");
	}
	VkBuffer staging_buffer;
	VkDeviceMemory stagingBufferMemory;
	VulkanUtility::CreateBuffer(
		vk_contex,
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer,
		stagingBufferMemory,
		1, nullptr);
	void* data;
	vkMapMemory(vk_contex.logical_device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(vk_contex.logical_device, stagingBufferMemory);
	stbi_image_free(pixels);
	VulkanUtility::CreateImage(
		vk_contex,
		texWidth,
		texHeight,
		miplevel_size,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		tex_dict[tex_name]->texture_image,
		tex_dict[tex_name]->texture_image_memory);
	VulkanUtility::TransitionImageLayout(vk_contex, tex_dict[tex_name]->texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, miplevel_size);
	VulkanUtility::CopyBufferToImage(vk_contex,staging_buffer, tex_dict[tex_name]->texture_image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	VulkanUtility::GenerateMipmaps(vk_contex, tex_dict[tex_name]->texture_image, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, miplevel_size);
	vkDestroyBuffer(vk_contex.logical_device, staging_buffer, nullptr);
	vkFreeMemory(vk_contex.logical_device, stagingBufferMemory, nullptr);
	tex_dict[tex_name]->texture_image_view = VulkanUtility::CreateImageView(vk_contex, tex_dict[tex_name]->texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, miplevel_size);
	CreateTextureSampler(vk_contex, *tex_dict[tex_name]);
}

void TextureMgr::CreateTextureSampler(const VulkanUtility::VulkanContex& vk_contex, Texture& tex) {
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(vk_contex.physical_device, &supportedFeatures);
	samplerInfo.anisotropyEnable = supportedFeatures.samplerAnisotropy;
	// The maxAnisotropy field limits the amount of texel samples that can be used to calculate the final color. 
	samplerInfo.maxAnisotropy = supportedFeatures.samplerAnisotropy == VK_TRUE ? 16 : 1;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(tex.miplevel_size);
	if (vkCreateSampler(vk_contex.logical_device, &samplerInfo, nullptr, &(tex.texture_sampler)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

shared_ptr<TextureMgr::Texture> TextureMgr::GetTextureByName(string tex_name) {
	return tex_dict[tex_name];
}

void TextureMgr::CleanUpTextures(const VulkanUtility::VulkanContex vk_contex) {

	for (auto kv : tex_dict) {
		vkDestroySampler(vk_contex.logical_device, kv.second->texture_sampler, nullptr);
		vkDestroyImageView(vk_contex.logical_device, kv.second->texture_image_view, nullptr);
		vkDestroyImage(vk_contex.logical_device, kv.second->texture_image, nullptr);
		vkFreeMemory(vk_contex.logical_device, kv.second->texture_image_memory, nullptr);
	}
	tex_dict.clear();
}