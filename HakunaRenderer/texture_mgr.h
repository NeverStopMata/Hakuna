#pragma once
#include <map>
#include <memory>
#include <algorithm>
#include "vulkan_core.h"
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
	std::map<std::string, shared_ptr<Texture>> tex_dict;
	TextureMgr();
	void CreateTextureImage(const VulkanCore::VulkanContex& vk_contex, string file_path, Texture& tex);
	void CreateTextureSampler(const VulkanCore::VulkanContex& vk_contex, Texture& tex);
	~TextureMgr();
private:

};

