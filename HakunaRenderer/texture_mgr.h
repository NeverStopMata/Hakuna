#pragma once
#include <map>
#include <memory>
#include <algorithm>
#include "vulkan_utility.h"
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
	};
	std::map<string, shared_ptr<Texture>> tex_dict;
	TextureMgr();
	void CreateTextureImage(const VulkanUtility::VulkanContex& vk_contex, string file_path, string tex_name);
	void CreateTextureSampler(const VulkanUtility::VulkanContex& vk_contex, Texture& tex);
	shared_ptr<Texture> GetTextureByName(string tex_name);
	void CleanUpTextures(const VulkanUtility::VulkanContex vk_contex);
	~TextureMgr();
private:

};

