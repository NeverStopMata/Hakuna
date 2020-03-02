#pragma once
#include <map>
#include <memory>
#include <algorithm>
#include "vulkan_utility.h"
#include "vulkan/vulkan.h"
#include <gli/gli.hpp>
using namespace std;
class Texture
{
public:
	VkImage texture_image_;
	VkDeviceMemory texture_image_memory_;
	VkImageView texture_image_view_;
	VkSampler texture_sampler_;
	uint32_t miplevel_size_;
	uint32_t width_, height_;
	uint32_t layerCount_;
	VkDescriptorImageInfo descriptor_;
	const VulkanUtility::VulkanContex* vk_contex_ptr_;
	Texture() = delete;

	void CleanUp()
	{
		vkDestroySampler(vk_contex_ptr_->vulkan_device.logical_device, texture_sampler_, nullptr);
		vkDestroyImageView(vk_contex_ptr_->vulkan_device.logical_device, texture_image_view_, nullptr);
		vkDestroyImage(vk_contex_ptr_->vulkan_device.logical_device, texture_image_, nullptr);
		vkFreeMemory(vk_contex_ptr_->vulkan_device.logical_device, texture_image_memory_, nullptr);
	}
	~Texture() {
	}
	Texture(const VulkanUtility::VulkanContex* vk_contex_ptr):vk_contex_ptr_(vk_contex_ptr) {}
	std::shared_ptr<Texture> LoadTexture2D(string file_path);
	std::shared_ptr<Texture> LoadTextureCube(string file_path);
};

class TextureMgr
{
public:
	static TextureMgr* GetInstance()
	{
		if (p_instance_ == nullptr)
			p_instance_ = new TextureMgr();
		return p_instance_;
	}
	void AddTexture(string tex_name, std::shared_ptr<Texture> tex_ptr);
	bool FindTextureByName(string tex_name);
	shared_ptr<Texture> GetTextureByName(string tex_name);
	~TextureMgr() 
	{
		CleanUpTextures();
	};
private:
	void CleanUpTextures();
	static TextureMgr* p_instance_;
	TextureMgr() {};
	TextureMgr(const TextureMgr&);
	TextureMgr& operator=(const TextureMgr&);
	std::map<string, shared_ptr<Texture>> tex_dict_;
	void CreateTextureSampler(const VulkanUtility::VulkanContex& vk_contex, Texture& tex);
};

