#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <set>
#include <array>
#include <chrono>
#include <climits>
#include "vk_swapchain.h"
#include "vertex.h"
#define M_PI 3.1415926
using namespace std;
const int MAX_FRAMES_IN_FLIGHT = 2;
const vector<const char*> validationLayers = {
"VK_LAYER_LUNARG_standard_validation"
};

const vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME//显式地设置swap chain 的扩展（虽然只要presentation的queue family存在就可以说明该物理设备支持输出到屏幕）
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif



class VulkanUtility {

public:
	struct QueueFamilyIndices {
		int graphicsFamily = -1;//绘制需要
		int presentFamily = -1; //需要将画面输出到屏幕
		int transferFamily = -1;
		bool IsComplete() {
			return graphicsFamily >= 0 &&
				presentFamily >= 0 &&
				transferFamily >= 0;
		}
	};//表示物理设备所支持的且需要的queue families的index.

	struct VulkanContex {
		VkInstance vk_instance;
		//VkSurfaceKHR surface;
		VkPhysicalDevice physical_device = VK_NULL_HANDLE;
		//The logical device should be destroyed in cleanup with the vkDestroyDevice function:
		VkDevice logical_device;
		VkQueue graphics_queue;
		VkQueue present_queue;
		VkQueue transfer_queue;
		QueueFamilyIndices queue_family_indices;
		VkCommandPool transfer_command_pool;
		VkCommandPool graphic_command_pool;
		vector<VkCommandBuffer> commandBuffers;//with the size of swapChain image count.
		VkSampleCountFlagBits msaasample_size;

		VkPipelineCache pipeline_cache;

		VKSwapchain swapchain;
		vector<VkFramebuffer> swapchain_framebuffers;//with the size of swapChain image count.
		VkRenderPass renderpass;
		VkPipelineLayout pipeline_layout;

		VkImage depth_image;
		VkDeviceMemory depth_image_memory;
		VkImageView depth_image_view;

		//used for multi-sampling
		VkImage color_image;
		VkDeviceMemory color_image_memory;
		VkImageView color_image_view;

		VkDescriptorPool descriptor_pool;
		VkDescriptorSetLayout descriptor_set_layout;
		struct PipeLineStruct {
			VkPipeline opaque_pipeline;
			VkPipeline skybox_pipeline;
		}pipeline_struct;
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;//Basic surface capabilities (min/max number of images in swap chain,	min / max width and height of images)
		vector<VkSurfaceFormatKHR> formats;//Surface formats (pixel format, color space)
		vector<VkPresentModeKHR> presentModes;
	};

public:

	static bool HasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	static void BeginSingleTimeCommands(const VulkanContex& vk_contex, uint32_t queueFamilyIndex, VkCommandBuffer& commandBuffer) {
		VkCommandPool commandPool;
		if (queueFamilyIndex == vk_contex.queue_family_indices.transferFamily) {
			commandPool = vk_contex.transfer_command_pool;
		}
		else if (queueFamilyIndex == vk_contex.queue_family_indices.graphicsFamily) {
			commandPool = vk_contex.graphic_command_pool;
		}
		else {
			throw std::invalid_argument("invalid queueFamilyIndex!");
		}
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		//VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(vk_contex.logical_device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		//return commandBuffer;
	}

	static void EndSingleTimeCommands(const VulkanContex& vk_contex, VkCommandBuffer commandBuffer, uint32_t queueFamilyIndex) {
		VkCommandPool commandPool;
		VkQueue queue;
		if (queueFamilyIndex == vk_contex.queue_family_indices.transferFamily) {
			commandPool = vk_contex.transfer_command_pool;
			queue = vk_contex.transfer_queue;
		}
		else if (queueFamilyIndex == vk_contex.queue_family_indices.graphicsFamily) {
			commandPool = vk_contex.graphic_command_pool;
			queue = vk_contex.graphics_queue;
		}
		else {
			throw std::invalid_argument("invalid queueFamilyIndex!");
		}
		vkEndCommandBuffer(commandBuffer);
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);
		//vkQueueWaitIdle(queue);//EMMMMMMM
		vkFreeCommandBuffers(vk_contex.logical_device, commandPool, 1, &commandBuffer);
	}

	static void CopyBufferToImage(const VulkanContex& vk_contex, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommands(vk_contex, static_cast<uint32_t>(vk_contex.queue_family_indices.transferFamily), commandBuffer);
		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};
		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);
		EndSingleTimeCommands(vk_contex, commandBuffer, static_cast<uint32_t>(vk_contex.queue_family_indices.transferFamily));
	}

	//static VkPipelineShaderStageCreateInfo LoadShader(const VulkanContex& vk_contex,std::string fileName, VkShaderStageFlagBits stage)
	//{
	//	VkPipelineShaderStageCreateInfo shaderStage = {};
	//	auto vertShaderCode = ShaderManager::ReadShaderFile(fileName);
	//	VkShaderModule vertShaderModule = VulkanUtility::CreateShaderModule(vk_contex, vertShaderCode);
	//	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	//	shaderStage.stage = stage;
	//	shaderStage.module = vertShaderModule;
	//	shaderStage.pName = "main";
	//	vkDestroyShaderModule(vk_contex.logical_device, vertShaderModule, nullptr);
	//	return shaderStage;
	//}
	

	static void setImageLayout(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkFormat format,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkImageSubresourceRange subresourceRange)
	{
		VkImageMemoryBarrier barrier = {};
		barrier.subresourceRange = subresourceRange;
		if (newImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (HasStencilComponent(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldImageLayout;
		barrier.newLayout = newImageLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else if (oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if (oldImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}
		vkCmdPipelineBarrier(
			cmdbuffer,
			sourceStage,
			destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

	}

	static void TransitionImageLayout(
		const VulkanContex& vk_contex, 
		VkImage image, VkFormat format, 
		VkImageLayout oldLayout, 
		VkImageLayout newLayout, 
		uint32_t mipLevels, 
		uint32_t layer_cnt = 1, 
		VkImageAspectFlagBits imageAspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT) {
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommands(vk_contex, static_cast<uint32_t>(vk_contex.queue_family_indices.graphicsFamily), commandBuffer);

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = imageAspect;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipLevels;
		subresourceRange.layerCount = layer_cnt;
		setImageLayout(commandBuffer, image, format, oldLayout, newLayout, subresourceRange);
		EndSingleTimeCommands(vk_contex, commandBuffer, static_cast<uint32_t>(vk_contex.queue_family_indices.graphicsFamily));
	}


	static void CreateTextureSampler(const VulkanUtility::VulkanContex& vk_contex,uint32_t numMips,VkSampler* result_sampler) {
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(vk_contex.physical_device, &supportedFeatures);
		samplerInfo.anisotropyEnable = supportedFeatures.samplerAnisotropy;
		// The maxAnisotropy field limits the amount of texel samples that can be used to calculate the final color. 
		samplerInfo.maxAnisotropy = supportedFeatures.samplerAnisotropy == VK_TRUE ? 16 : 1;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(numMips);
		if (vkCreateSampler(vk_contex.logical_device, &samplerInfo, nullptr, result_sampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	static void CreateImageView(VulkanContex vk_contex, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageViewType view_image_type, VkImageView* imageViewPtr) {
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; 
		viewInfo.image = image; 
		viewInfo.viewType = view_image_type;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = view_image_type == VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

		VkImageView imageView;
		if (vkCreateImageView(vk_contex.logical_device, &viewInfo, nullptr, imageViewPtr) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
	}

	static uint32_t FindMemoryType(VkPhysicalDevice physical_device, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		return -1;
	}

	static void CreateImage(
		const VulkanContex& vk_contex,
		uint32_t width,
		uint32_t height,
		uint32_t mipLevels,
		bool bIsCubemap,
		VkSampleCountFlagBits numSamples,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory&imageMemory) {
		VkImageCreateInfo imageCI = {};
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.extent.width = width;
		imageCI.extent.height = height;
		imageCI.extent.depth = 1;
		imageCI.mipLevels = mipLevels;
		imageCI.arrayLayers = bIsCubemap?6:1;
		imageCI.format = format;
		imageCI.tiling = tiling;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCI.usage = usage;
		imageCI.samples = numSamples;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCI.flags = bIsCubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
		if (vkCreateImage(vk_contex.logical_device, &imageCI, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(vk_contex.logical_device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(vk_contex.physical_device, memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(vk_contex.logical_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(vk_contex.logical_device, image, imageMemory, 0);
	}



	static void GenerateMipmaps(const VulkanContex& vk_contex, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {

		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(vk_contex.physical_device, imageFormat, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			throw std::runtime_error("texture image format does not support linear blitting!");
		}
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommands(vk_contex, static_cast<uint32_t>(vk_contex.queue_family_indices.graphicsFamily), commandBuffer);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for (uint32_t i = 1; i < mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit = {};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VkFilter::VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}
		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		EndSingleTimeCommands(vk_contex, commandBuffer, static_cast<uint32_t>(vk_contex.queue_family_indices.graphicsFamily));
	}

	static void CreateBuffer(
		const VulkanContex& vk_contex,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory,
		int sharingQueueFamilyCount,
		const uint32_t * psharingQueueFamilyIndices) {
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		if (sharingQueueFamilyCount > 1) {
			bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
			bufferInfo.pQueueFamilyIndices = psharingQueueFamilyIndices;
			bufferInfo.queueFamilyIndexCount = sharingQueueFamilyCount;
		}
		else {
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		if (vkCreateBuffer(vk_contex.logical_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(vk_contex.logical_device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(vk_contex.physical_device, memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(vk_contex.logical_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}
		vkBindBufferMemory(vk_contex.logical_device, buffer, bufferMemory, 0);
	}

	static bool CheckValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		for (const char* layerName : validationLayers) {
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}
			if (!layerFound) {
				return false;
			}
		}
		return true;
	}

	static std::vector<const char*> GetRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	static void CreateInstance(VulkanContex& vk_contex) {
		if (enableValidationLayers && !CheckValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		auto extensions = GetRequiredExtensions();
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
		if (enableValidationLayers) {
			instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			instanceCreateInfo.enabledLayerCount = 0;
		}
		if (vkCreateInstance(&instanceCreateInfo, nullptr, &(vk_contex.vk_instance)) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	//static void CreateSurface(VulkanContex& vk_contex, GLFWwindow* windows) {
	//	if (glfwCreateWindowSurface(vk_contex.vk_instance, windows, nullptr, &(vk_contex.surface)) != VK_SUCCESS) {
	//		throw std::runtime_error("failed to create window surface!");
	//	}
	//}

	static void CreateLogicalDevice(VulkanContex& vk_contex)
	{
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		//If the queue families are the same, then we only need to pass its index once.
		std::set<int> uniqueQueueFamilies = { vk_contex.queue_family_indices.graphicsFamily, vk_contex.queue_family_indices.presentFamily,vk_contex.queue_family_indices.transferFamily };
		float queuePriority = 1.0f;
		for (int queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}
		VkPhysicalDeviceFeatures deviceFeatures = {};//no need for any specific feature
		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(vk_contex.physical_device, &supportedFeatures);
		deviceFeatures.samplerAnisotropy = supportedFeatures.samplerAnisotropy;
		deviceFeatures.sampleRateShading = VK_TRUE; // enable sample shading feature for the device
		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		if (enableValidationLayers) {
			deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			deviceCreateInfo.enabledLayerCount = 0;
		}
		if (vkCreateDevice(vk_contex.physical_device, &deviceCreateInfo, nullptr, &vk_contex.logical_device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}
		vkGetDeviceQueue(vk_contex.logical_device, static_cast<uint32_t>(vk_contex.queue_family_indices.graphicsFamily), 0, &vk_contex.graphics_queue);
		vkGetDeviceQueue(vk_contex.logical_device, static_cast<uint32_t>(vk_contex.queue_family_indices.presentFamily), 0, &vk_contex.present_queue);
		vkGetDeviceQueue(vk_contex.logical_device, static_cast<uint32_t>(vk_contex.queue_family_indices.transferFamily), 0, &vk_contex.transfer_queue);
	}

	static bool CheckDeviceExtensionsSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}
		return requiredExtensions.empty();
	}

	static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
		QueueFamilyIndices indices;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies;
		if (queueFamilyCount != 0) {
			queueFamilies.resize(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyCount, queueFamilies.data());
		}
		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			//find the queue family that surpports Graphic cmd
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}
			//find the queue family that surpports presentation(render to a rt)
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &presentSupport);
			if (queueFamily.queueCount > 0 && presentSupport) {
				indices.presentFamily = i;
			}

			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
				indices.transferFamily = i;
			}
			if (indices.IsComplete()) {
				break;
			}
			i++;
		}
		//std::cout << indices.graphicsFamily << " " << indices.transferFamily << " " << indices.presentFamily << std::endl;;

		return indices;
	}

	static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.capabilities);
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &presentModeCount, details.presentModes.data());
		}
		return details;
	}

	static bool IsDeviceSuitable(VkPhysicalDevice physical_device, VulkanContex& vk_contex) {
		VulkanUtility::QueueFamilyIndices indices = FindQueueFamilies(physical_device, vk_contex.swapchain.surface_);
		bool extensionsSupported = CheckDeviceExtensionsSupport(physical_device);
		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physical_device, vk_contex.swapchain.surface_);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}
		bool ret = indices.IsComplete() && extensionsSupported&& swapChainAdequate;
		if (ret == true) {
			vk_contex.queue_family_indices = indices;
		}


		return indices.IsComplete() && extensionsSupported&& swapChainAdequate;
	}

	static VkSampleCountFlagBits GetMaxUsableSampleCount(const VulkanContex& vk_contex) {
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(vk_contex.physical_device, &physicalDeviceProperties);

		VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}

	static void PickPhysicalDevice(VulkanContex& vk_contex) {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(vk_contex.vk_instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkansupport!");
		}
		std::vector<VkPhysicalDevice> physical_devices(deviceCount);
		vkEnumeratePhysicalDevices(vk_contex.vk_instance, &deviceCount, physical_devices.data());
		for (const auto& physical_device : physical_devices) {
			if (IsDeviceSuitable(physical_device, vk_contex)) {
				vk_contex.physical_device = physical_device;
				vk_contex.msaasample_size = GetMaxUsableSampleCount(vk_contex);
				break;
			}
		}
		if (vk_contex.physical_device == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable CPU!");
		}
	}

	static VkFormat FindSupportedFormat(const VulkanContex& vk_contex, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(vk_contex.physical_device, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}
	static VkFormat FindDepthFormat(const VulkanContex& vk_contex) {
		return FindSupportedFormat(vk_contex,
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}
	static void CreateRenderPass(VulkanContex& vk_contex) {
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = vk_contex.swapchain.colorFormat_;
		colorAttachment.samples = vk_contex.msaasample_size;
		/*The loadOp and storeOp apply to color and depth data, and stencilLoadOp / stencilStoreOp apply to stencil data.*/
		/*The loadOp and storeOp determine what to do with the data in the attachment before rendering and after rendering*/
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;// we don't care what previous layout the image was in.
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef = {};
		/*index The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!*/
		colorAttachmentRef.attachment = 0;//attachment 
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = FindDepthFormat(vk_contex);
		depthAttachment.samples = vk_contex.msaasample_size;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve = {};
		colorAttachmentResolve.format = vk_contex.swapchain.colorFormat_;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentResolveRef = {};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;//表示上一个subpass(若该宏被dstSubpass使用，则表示下一个subpass.
		dependency.dstSubpass = 0;//表示本subpass本身
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;//等待被依赖对象的color attachment output阶段
		dependency.srcAccessMask = 0;//无特定操作
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;//被阻塞的subpass也是color attachment output这个阶段依赖于src
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;//上述阶段中涉及到的操作有读写

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;

		std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;
		if (vkCreateRenderPass(vk_contex.logical_device, &renderPassInfo, nullptr, &vk_contex.renderpass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	static void CopyBuffer(const VulkanContex& vk_contex, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommands(vk_contex, static_cast<uint32_t>(vk_contex.queue_family_indices.transferFamily), commandBuffer);
		VkBufferCopy copyRegion = {};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
		EndSingleTimeCommands(vk_contex, commandBuffer, static_cast<uint32_t>(vk_contex.queue_family_indices.transferFamily));
	}

	static void CreateDescriptorPool(VulkanContex& vk_contex) {
		std::array<VkDescriptorPoolSize, 2> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(vk_contex.swapchain.imageCount_ * 6);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(vk_contex.swapchain.imageCount_ * 16);
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(vk_contex.swapchain.imageCount_ * 2);
		if (vkCreateDescriptorPool(vk_contex.logical_device, &poolInfo, nullptr, &vk_contex.descriptor_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	static inline VkWriteDescriptorSet GetWriteDescriptorSet(
		VkDescriptorSet dstSet,
		VkDescriptorType type,
		uint32_t binding,
		VkDescriptorBufferInfo* bufferInfo,
		uint32_t descriptorCount = 1)
	{
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = dstSet;
		writeDescriptorSet.descriptorType = type;
		writeDescriptorSet.dstBinding = binding;
		writeDescriptorSet.pBufferInfo = bufferInfo;
		writeDescriptorSet.descriptorCount = descriptorCount;
		return writeDescriptorSet;
	}

	static inline VkDescriptorSetLayoutBinding GetDescriptorSetLayoutBinding(
		VkDescriptorType type,
		VkShaderStageFlags stageFlags,
		uint32_t binding,
		uint32_t descriptorCount = 1)
	{
		VkDescriptorSetLayoutBinding setLayoutBinding{};
		setLayoutBinding.descriptorType = type;
		setLayoutBinding.stageFlags = stageFlags;
		setLayoutBinding.binding = binding;
		setLayoutBinding.descriptorCount = descriptorCount;
		return setLayoutBinding;
	}

	static void CreateDescriptorSetLayout(VulkanContex& vk_contex) {
		std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
			GetDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
			GetDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
			GetDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 2),
			GetDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
			GetDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
			GetDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
			GetDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6),
			GetDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 7),
			GetDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 8),
			GetDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 9),
			GetDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 10),
		};

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(set_layout_bindings.size());
		layoutInfo.pBindings = set_layout_bindings.data();

		if (vkCreateDescriptorSetLayout(vk_contex.logical_device, &layoutInfo, nullptr, &vk_contex.descriptor_set_layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	static VkShaderModule CreateShaderModule(const VulkanContex& vk_contex, const std::vector<char>& code) {
		VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.codeSize = code.size();
		shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(vk_contex.logical_device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}
		return shaderModule;
	}



	static void CreateCommandPools(VulkanContex& vk_contex) {
		VkCommandPoolCreateInfo graphicPoolInfo = {};
		graphicPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		graphicPoolInfo.queueFamilyIndex = static_cast<uint32_t>(vk_contex.queue_family_indices.graphicsFamily);
		graphicPoolInfo.flags = 0; // Optional
		if (vkCreateCommandPool(vk_contex.logical_device, &graphicPoolInfo, nullptr, &vk_contex.graphic_command_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphic command pool!");
		}

		VkCommandPoolCreateInfo transferPoolInfo = {};
		transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		transferPoolInfo.queueFamilyIndex = static_cast<uint32_t>(vk_contex.queue_family_indices.transferFamily);
		transferPoolInfo.flags = 0; // Optional
		if (vkCreateCommandPool(vk_contex.logical_device, &transferPoolInfo, nullptr, &vk_contex.transfer_command_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create transfer command pool!");
		}
	}

	static void CreateColorResources(VulkanContex& vk_contex) {
		VkFormat colorFormat = vk_contex.swapchain.colorFormat_;
		VulkanUtility::CreateImage(
			vk_contex,
			vk_contex.swapchain.extent_.width,
			vk_contex.swapchain.extent_.height,
			1,
			false,
			vk_contex.msaasample_size,
			colorFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vk_contex.color_image,
			vk_contex.color_image_memory);
		VulkanUtility::CreateImageView(vk_contex, vk_contex.color_image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_IMAGE_VIEW_TYPE_2D,&vk_contex.color_image_view);
		TransitionImageLayout(vk_contex, vk_contex.color_image, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	}

	static void CreateDepthResources(VulkanContex& vk_contex) {
		VkFormat depthFormat = FindDepthFormat(vk_contex);
		VulkanUtility::CreateImage(
			vk_contex,
			vk_contex.swapchain.extent_.width,
			vk_contex.swapchain.extent_.height,
			1,
			false,
			vk_contex.msaasample_size,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vk_contex.depth_image,
			vk_contex.depth_image_memory
		);
		VulkanUtility::CreateImageView(vk_contex, vk_contex.depth_image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_IMAGE_VIEW_TYPE_2D,&vk_contex.depth_image_view);
		TransitionImageLayout(vk_contex, vk_contex.depth_image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
	}

	static void CreateFramebuffers(VulkanContex& vk_contex) {
		vk_contex.swapchain_framebuffers.resize(vk_contex.swapchain.imageCount_);
		for (size_t i = 0; i < vk_contex.swapchain.imageCount_; i++) {
			std::array<VkImageView, 3> attachments = {
				/*The color attachment differs for every swap chain image,
				but the same depth image can be used by all of them
				because only a single subpass is running at the same time due to our semaphores.*/
							vk_contex.color_image_view,
							vk_contex.depth_image_view,
							vk_contex.swapchain.buffers_[i].view
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = vk_contex.renderpass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = vk_contex.swapchain.extent_.width;
			framebufferInfo.height = vk_contex.swapchain.extent_.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(vk_contex.logical_device, &framebufferInfo, nullptr, &vk_contex.swapchain_framebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	static void CleanupFramebufferAndPipeline(VulkanContex& vk_contex, vector<VkCommandBuffer>& commandBuffers) {
		for (size_t i = 0; i < vk_contex.swapchain_framebuffers.size(); i++) {
			vkDestroyFramebuffer(vk_contex.logical_device, vk_contex.swapchain_framebuffers[i], nullptr);
		}
		vkDestroyImageView(vk_contex.logical_device, vk_contex.color_image_view, nullptr);
		vkDestroyImage(vk_contex.logical_device, vk_contex.color_image, nullptr);
		vkFreeMemory(vk_contex.logical_device, vk_contex.color_image_memory, nullptr);
		vkDestroyImageView(vk_contex.logical_device, vk_contex.depth_image_view, nullptr);
		vkDestroyImage(vk_contex.logical_device, vk_contex.depth_image, nullptr);
		vkFreeMemory(vk_contex.logical_device, vk_contex.depth_image_memory, nullptr);
		/* This way we can reuse the existing pool to allocate the new command buffers*/
		vkFreeCommandBuffers(vk_contex.logical_device, vk_contex.graphic_command_pool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		vkDestroyPipelineLayout(vk_contex.logical_device, vk_contex.pipeline_layout, nullptr);
		vkDestroyRenderPass(vk_contex.logical_device, vk_contex.renderpass, nullptr);
		vkDestroyPipeline(vk_contex.logical_device, vk_contex.pipeline_struct.opaque_pipeline, nullptr);
		vkDestroyPipeline(vk_contex.logical_device, vk_contex.pipeline_struct.skybox_pipeline, nullptr);
	}
	
};