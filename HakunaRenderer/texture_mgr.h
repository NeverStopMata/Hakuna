#pragma once
#include <map>
#include <memory>
#include <algorithm>
#include "vulkan_utility.h"
#include "vulkan/vulkan.h"
#include <gli/gli.hpp>
using namespace std;
class TextureMgr
{
public:
	struct Texture
	{
		VkImage texture_image;
		VkDeviceMemory texture_image_memory;
		VkImageView texture_image_view;
		VkSampler texture_sampler;
		uint32_t miplevel_size;
		uint32_t width, height;
		uint32_t layerCount;
	};
	std::map<string, shared_ptr<Texture>> tex_dict_;
	TextureMgr();
	void CreateTexture2D(const VulkanUtility::VulkanContex& vk_contex, VkFormat format, string file_path, string tex_name);
	void CreateTextureCube(const VulkanUtility::VulkanContex& vk_contex, VkFormat format, string file_path, string tex_name);
	shared_ptr<Texture> GetTextureByName(string tex_name);
	void CleanUpTextures(const VulkanUtility::VulkanContex& vk_contex);
	~TextureMgr();
private:
	void CreateTextureSampler(const VulkanUtility::VulkanContex& vk_contex, Texture& tex);

};

