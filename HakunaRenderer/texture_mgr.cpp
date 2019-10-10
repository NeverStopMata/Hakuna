#include "texture_mgr.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

TextureMgr::TextureMgr()
{
}

TextureMgr::~TextureMgr()
{
}

std::shared_ptr<TextureMgr::Texture> TextureMgr::LoadTexture2D(const VulkanUtility::VulkanContex& vk_contex, VkFormat format, string file_path) {
	auto texture_ptr = make_shared<Texture>();
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(file_path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	uint32_t miplevel_size = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	texture_ptr->miplevel_size = miplevel_size;
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (pixels == NULL) {
		throw std::runtime_error("failed to load texture image!");
	}
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	VulkanUtility::CreateBuffer(
		vk_contex,
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer,
		staging_buffer_memory,
		1, nullptr);
	void* data;
	vkMapMemory(vk_contex.logical_device, staging_buffer_memory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(vk_contex.logical_device, staging_buffer_memory);
	stbi_image_free(pixels);
	VulkanUtility::CreateImage(
		vk_contex,
		texWidth,
		texHeight,
		miplevel_size,
		false,
		VK_SAMPLE_COUNT_1_BIT,
		format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		texture_ptr->texture_image,
		texture_ptr->texture_image_memory);
	VulkanUtility::TransitionImageLayout(vk_contex, texture_ptr->texture_image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, miplevel_size);
	VulkanUtility::CopyBufferToImage(vk_contex,staging_buffer, texture_ptr->texture_image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	VulkanUtility::GenerateMipmaps(vk_contex, texture_ptr->texture_image, format, texWidth, texHeight, miplevel_size);
	vkDestroyBuffer(vk_contex.logical_device, staging_buffer, nullptr);
	vkFreeMemory(vk_contex.logical_device, staging_buffer_memory, nullptr);
	VulkanUtility::CreateImageView(vk_contex, texture_ptr->texture_image, format, VK_IMAGE_ASPECT_COLOR_BIT, miplevel_size, VkImageViewType::VK_IMAGE_VIEW_TYPE_2D, &texture_ptr->texture_image_view);
	VulkanUtility::CreateTextureSampler(vk_contex, texture_ptr ->miplevel_size,&(texture_ptr->texture_sampler));	
	texture_ptr->descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texture_ptr->descriptor.imageView = texture_ptr->texture_image_view;
	texture_ptr->descriptor.sampler = texture_ptr->texture_sampler;
	return texture_ptr;
}

std::shared_ptr<TextureMgr::Texture> TextureMgr::LoadTextureCube(const VulkanUtility::VulkanContex& vk_contex, VkFormat format, string file_path) {
	gli::texture_cube tex_cube(gli::load(file_path));
	assert(!tex_cube.empty());
	auto image_size = tex_cube.size();
	auto texture_cubemap = make_shared<Texture>();
	texture_cubemap->width = static_cast<uint32_t>(tex_cube.extent().x);
	texture_cubemap->height = static_cast<uint32_t>(tex_cube.extent().y);
	texture_cubemap->miplevel_size = static_cast<uint32_t>(tex_cube.levels());

	// Create a host-visible staging buffer that contains the raw image data
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	VulkanUtility::CreateBuffer(
		vk_contex,
		image_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer,
		staging_buffer_memory,
		1, nullptr);
	// Copy texture data into staging buffer
	void *data;
	vkMapMemory(vk_contex.logical_device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, tex_cube.data(), static_cast<size_t>(image_size));
	vkUnmapMemory(vk_contex.logical_device, staging_buffer_memory);

	// Setup buffer copy regions for each face including all of it's miplevels
	std::vector<VkBufferImageCopy> bufferCopyRegions;
	size_t offset = 0;
	for (uint32_t face = 0; face < 6; face++)
	{
		for (uint32_t level = 0; level < texture_cubemap->miplevel_size; level++)
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
	image_create_info.mipLevels = texture_cubemap->miplevel_size;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.extent = { texture_cubemap->width, texture_cubemap->height, 1 };
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


	if (vkCreateImage(vk_contex.logical_device, &image_create_info, nullptr, &texture_cubemap->texture_image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}
	
	VkMemoryAllocateInfo mem_alloc_info{};
	mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(vk_contex.logical_device, texture_cubemap->texture_image, &mem_reqs);

	mem_alloc_info.allocationSize = mem_reqs.size;
	mem_alloc_info.memoryTypeIndex = VulkanUtility::FindMemoryType(vk_contex.physical_device, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(vk_contex.logical_device, &mem_alloc_info, nullptr, &texture_cubemap->texture_image_memory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}
	vkBindImageMemory(vk_contex.logical_device, texture_cubemap->texture_image, texture_cubemap->texture_image_memory, 0);

	VulkanUtility::TransitionImageLayout(
		vk_contex, 
		texture_cubemap->texture_image, 
		format, 
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		texture_cubemap->miplevel_size, 
		static_cast<uint32_t>(6)
	);
	VkCommandBuffer commandBuffer = VulkanUtility::BeginSingleTimeCommands(vk_contex, static_cast<uint32_t>(vk_contex.queue_family_indices.transferFamily));
	vkCmdCopyBufferToImage(
		commandBuffer,
		staging_buffer,
		texture_cubemap->texture_image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		static_cast<uint32_t>(bufferCopyRegions.size()),
		bufferCopyRegions.data());
	VulkanUtility::EndSingleTimeCommands(vk_contex, commandBuffer, static_cast<uint32_t>(vk_contex.queue_family_indices.transferFamily));
	
	VulkanUtility::TransitionImageLayout(
		vk_contex,
		texture_cubemap->texture_image,
		format,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		texture_cubemap->miplevel_size,
		static_cast<uint32_t>(6)
	);
	vkDestroyBuffer(vk_contex.logical_device, staging_buffer, nullptr);
	vkFreeMemory(vk_contex.logical_device, staging_buffer_memory, nullptr);
	VulkanUtility::CreateImageView(vk_contex, texture_cubemap->texture_image, format, VK_IMAGE_ASPECT_COLOR_BIT, texture_cubemap->miplevel_size, VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE,&texture_cubemap->texture_image_view);
	VulkanUtility::CreateTextureSampler(vk_contex, texture_cubemap->miplevel_size, &(texture_cubemap->texture_sampler));
	texture_cubemap->descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texture_cubemap->descriptor.imageView = texture_cubemap->texture_image_view;
	texture_cubemap->descriptor.sampler = texture_cubemap->texture_sampler;
	return texture_cubemap;
}

void TextureMgr::AddTexture(string tex_name, shared_ptr<Texture> tex_ptr)
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
	vkGetPhysicalDeviceFeatures(vk_contex.physical_device, &supportedFeatures);
	samplerInfo.anisotropyEnable = supportedFeatures.samplerAnisotropy;
	// The maxAnisotropy field limits the amount of texel samples that can be used to calculate the final color. 
	samplerInfo.maxAnisotropy = supportedFeatures.samplerAnisotropy == VK_TRUE ? 16 : 1;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(tex.miplevel_size);
	if (vkCreateSampler(vk_contex.logical_device, &samplerInfo, nullptr, &(tex.texture_sampler)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

shared_ptr<TextureMgr::Texture> TextureMgr::GetTextureByName(string tex_name) {
	return tex_dict_[tex_name];
}

void TextureMgr::CleanUpTextures(const VulkanUtility::VulkanContex& vk_contex) {

	for (auto kv : tex_dict_) {
		vkDestroySampler(vk_contex.logical_device, kv.second->texture_sampler, nullptr);
		vkDestroyImageView(vk_contex.logical_device, kv.second->texture_image_view, nullptr);
		vkDestroyImage(vk_contex.logical_device, kv.second->texture_image, nullptr);
		vkFreeMemory(vk_contex.logical_device, kv.second->texture_image_memory, nullptr);
	}
	tex_dict_.clear();
}