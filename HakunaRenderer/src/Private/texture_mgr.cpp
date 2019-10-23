#include <iostream>
#include "texture_mgr.h"


void TextureMgr::AddTexture(string tex_name, std::shared_ptr<Texture> tex_ptr)
{
	tex_dict_[tex_name] = tex_ptr;
}

void TextureMgr::CreateTextureSampler(const VulkanUtility::VulkanContex& vk_contex, Texture& tex) {
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(vk_contex.vulkan_device.physical_device, &supportedFeatures);
	samplerInfo.anisotropyEnable = supportedFeatures.samplerAnisotropy;
	// The maxAnisotropy field limits the amount of texel samples that can be used to calculate the final color. 
	samplerInfo.maxAnisotropy = supportedFeatures.samplerAnisotropy == VK_TRUE ? 16 : 1;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(tex.miplevel_size_);
	if (vkCreateSampler(vk_contex.vulkan_device.logical_device, &samplerInfo, nullptr, &(tex.texture_sampler_)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

shared_ptr<Texture> TextureMgr::GetTextureByName(string tex_name) {
	return tex_dict_[tex_name];
}

void TextureMgr::CleanUpTextures(const VulkanUtility::VulkanContex& vk_contex) {


	auto test1 = tex_dict_["basecolor"].use_count();
	for (auto kv : tex_dict_) {
		kv.second->CleanUp();
		kv.second.reset();
	}
	tex_dict_.clear();
}


std::shared_ptr<Texture> Texture::LoadTexture2D(string file_path)
{
	gli::texture2d tex_2d(gli::load(file_path));
	VkFormat format = static_cast<VkFormat>(tex_2d.format());
	assert(!tex_2d.empty());
	auto image_size = tex_2d.size();
	this->width_ = static_cast<uint32_t>(tex_2d.extent().x);
	this->height_ = static_cast<uint32_t>(tex_2d.extent().y);
	this->miplevel_size_ = static_cast<uint32_t>(tex_2d.levels());

	VulkanBuffer stagingBuffer;
	vk_contex_ptr_->vulkan_device.CreateBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		image_size);
	stagingBuffer.map(image_size);
	memcpy(stagingBuffer.mapped, tex_2d.data(), tex_2d.size());
	stagingBuffer.unmap();
	VulkanUtility::CreateImage(
		*vk_contex_ptr_,
		this->width_,
		this->height_,
		this->miplevel_size_,
		false,
		VK_SAMPLE_COUNT_1_BIT,
		format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->texture_image_,
		this->texture_image_memory_);
	VulkanUtility::TransitionImageLayout(*vk_contex_ptr_, this->texture_image_, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, this->miplevel_size_);
	VulkanUtility::CopyBufferToImage(*vk_contex_ptr_, stagingBuffer.buffer, this->texture_image_, tex_2d);
	//VulkanUtility::GenerateMipmaps(*vk_contex_ptr_, this->texture_image_, format, this->width_, this->height_, this->miplevel_size_);
	VulkanUtility::TransitionImageLayout(*vk_contex_ptr_, this->texture_image_, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, this->miplevel_size_);
	stagingBuffer.destroy();
	VulkanUtility::CreateImageView(*vk_contex_ptr_, this->texture_image_, format, VK_IMAGE_ASPECT_COLOR_BIT, this->miplevel_size_, VkImageViewType::VK_IMAGE_VIEW_TYPE_2D, &this->texture_image_view_);
	VulkanUtility::CreateTextureSampler(*vk_contex_ptr_, this->miplevel_size_, &(this->texture_sampler_));
	this->descriptor_.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	this->descriptor_.imageView = this->texture_image_view_;
	this->descriptor_.sampler = this->texture_sampler_;
	return std::shared_ptr<Texture>(this);
}

std::shared_ptr<Texture> Texture::LoadTextureCube(string file_path)
{

	gli::texture_cube tex_cube(gli::load(file_path));
	VkFormat format = static_cast<VkFormat>(tex_cube.format());
	assert(!tex_cube.empty());
	auto image_size = tex_cube.size();
	this->width_ = static_cast<uint32_t>(tex_cube.extent().x);
	this->height_ = static_cast<uint32_t>(tex_cube.extent().y);
	this->miplevel_size_ = static_cast<uint32_t>(tex_cube.levels());

	//// Create a host-visible staging buffer that contains the raw image data
	VulkanBuffer stagingBuffer;
	vk_contex_ptr_->vulkan_device.CreateBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		image_size);
	stagingBuffer.map(image_size);
	memcpy(stagingBuffer.mapped, tex_cube.data(), static_cast<size_t>(image_size));
	stagingBuffer.unmap();

	// Setup buffer copy regions for each face including all of it's miplevels
	std::vector<VkBufferImageCopy> bufferCopyRegions;
	size_t offset = 0;
	for (uint32_t face = 0; face < 6; face++)
	{
		for (uint32_t level = 0; level < this->miplevel_size_; level++)
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = level;
			bufferCopyRegion.imageSubresource.baseArrayLayer = face;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(tex_cube[face][level].extent().x);
			bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(tex_cube[face][level].extent().y);
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;
			bufferCopyRegions.push_back(bufferCopyRegion);
			// Increase offset into staging buffer for next level / face
			offset += tex_cube[face][level].size();
		}
	}
	// Create optimal tiled target image
	VkImageCreateInfo image_create_info = {};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.format = format;
	image_create_info.mipLevels = this->miplevel_size_;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.extent = { this->width_, this->height_, 1 };
	image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	// Ensure that the TRANSFER_DST bit is set for staging
	if (!(image_create_info.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
	{
		image_create_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	// Cube faces count as array layers in Vulkan
	image_create_info.arrayLayers = 6;
	// This flag is required for cube map images
	image_create_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;


	if (vkCreateImage(vk_contex_ptr_->vulkan_device.logical_device, &image_create_info, nullptr, &this->texture_image_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryAllocateInfo mem_alloc_info{};
	mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(vk_contex_ptr_->vulkan_device.logical_device, this->texture_image_, &mem_reqs);

	mem_alloc_info.allocationSize = mem_reqs.size;
	mem_alloc_info.memoryTypeIndex = vk_contex_ptr_->vulkan_device.GetMemoryType(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(vk_contex_ptr_->vulkan_device.logical_device, &mem_alloc_info, nullptr, &this->texture_image_memory_) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}
	vkBindImageMemory(vk_contex_ptr_->vulkan_device.logical_device, this->texture_image_, this->texture_image_memory_, 0);

	VulkanUtility::TransitionImageLayout(
		*vk_contex_ptr_,
		this->texture_image_,
		format,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		this->miplevel_size_,
		static_cast<uint32_t>(6)
	);
	VkCommandBuffer commandBuffer;
	vk_contex_ptr_->vulkan_device.BeginSingleTimeCommands(static_cast<uint32_t>(vk_contex_ptr_->vulkan_device.queue_family_indices.transferFamily), commandBuffer);
	vkCmdCopyBufferToImage(
		commandBuffer,
		stagingBuffer.buffer,
		this->texture_image_,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		static_cast<uint32_t>(bufferCopyRegions.size()),
		bufferCopyRegions.data());
	vk_contex_ptr_->vulkan_device.EndSingleTimeCommands(commandBuffer, static_cast<uint32_t>(vk_contex_ptr_->vulkan_device.queue_family_indices.transferFamily));

	VulkanUtility::TransitionImageLayout(
		*vk_contex_ptr_,
		this->texture_image_,
		format,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		this->miplevel_size_,
		static_cast<uint32_t>(6)
	);
	stagingBuffer.destroy();
	VulkanUtility::CreateImageView(*vk_contex_ptr_, this->texture_image_, format, VK_IMAGE_ASPECT_COLOR_BIT, this->miplevel_size_, VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE, &this->texture_image_view_);
	VulkanUtility::CreateTextureSampler(*vk_contex_ptr_, this->miplevel_size_, &(this->texture_sampler_));
	this->descriptor_.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	this->descriptor_.imageView = this->texture_image_view_;
	this->descriptor_.sampler = this->texture_sampler_;
	return std::shared_ptr<Texture>(this);
}
