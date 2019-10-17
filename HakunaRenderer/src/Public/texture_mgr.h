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
		VkDescriptorImageInfo descriptor;
	};
	std::map<string, shared_ptr<Texture>> tex_dict_;
	TextureMgr();
	std::shared_ptr<TextureMgr::Texture> LoadTexture2D(const VulkanUtility::VulkanContex& vk_contex, VkFormat format, string file_path);
	std::shared_ptr<TextureMgr::Texture> LoadTextureCube(const VulkanUtility::VulkanContex& vk_contex, VkFormat format, string file_path);
	void AddTexture(string tex_name, shared_ptr<Texture> tex_ptr);
	shared_ptr<Texture> GetTextureByName(string tex_name);
	void CleanUpTextures(const VulkanUtility::VulkanContex& vk_contex);
	~TextureMgr();
private:
	void CreateTextureSampler(const VulkanUtility::VulkanContex& vk_contex, Texture& tex);

};

