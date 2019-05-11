#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <set>
using namespace std;
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

class VulkanCore {

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
		VkSurfaceKHR surface;
		VkPhysicalDevice physical_device = VK_NULL_HANDLE;
		//The logical device should be destroyed in cleanup with the vkDestroyDevice function:
		VkDevice logical_device;
		VkQueue graphics_queue;
		VkQueue present_queue;
		VkQueue transfer_queue;
		QueueFamilyIndices queue_family_indices;
		VkCommandPool transfer_command_pool;
		VkCommandPool graphic_command_pool;
		VkSampleCountFlagBits msaasample_size = VK_SAMPLE_COUNT_1_BIT;
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




	static VkCommandBuffer BeginSingleTimeCommands(const VulkanContex& vk_contex, uint32_t queueFamilyIndex) {
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

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(vk_contex.logical_device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
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

		vkFreeCommandBuffers(vk_contex.logical_device, commandPool, 1, &commandBuffer);
	}


	static void CopyBufferToImage(const VulkanContex& vk_contex, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(vk_contex, static_cast<uint32_t>(vk_contex.queue_family_indices.transferFamily));
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

	static void TransitionImageLayout(const VulkanContex& vk_contex, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(vk_contex, static_cast<uint32_t>(vk_contex.queue_family_indices.graphicsFamily));

		VkImageMemoryBarrier barrier = {};
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (HasStencilComponent(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}
		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
		EndSingleTimeCommands(vk_contex, commandBuffer, static_cast<uint32_t>(vk_contex.queue_family_indices.graphicsFamily));
	}

	static VkImageView CreateImageView(VulkanContex vk_contex, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(vk_contex.logical_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
		return imageView;
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
		VkSampleCountFlagBits numSamples,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory&
		imageMemory) {
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = numSamples;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(vk_contex.logical_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
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
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(vk_contex, static_cast<uint32_t>(vk_contex.queue_family_indices.graphicsFamily));

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

	static void CreateSurface(VulkanContex& vk_contex, GLFWwindow* windows) {
		if (glfwCreateWindowSurface(vk_contex.vk_instance, windows, nullptr, &(vk_contex.surface)) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

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

	static bool CheckDeviceExtensionsSupport(VkPhysicalDevice device){
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

	static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physical_device, VkSurfaceKHR surface){
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

	static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physical_device, VkSurfaceKHR surface){
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
		VulkanCore::QueueFamilyIndices indices = FindQueueFamilies(physical_device, vk_contex.surface);
		bool extensionsSupported = CheckDeviceExtensionsSupport(physical_device);
		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physical_device, vk_contex.surface);
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
			if (IsDeviceSuitable(physical_device,vk_contex)) {
				vk_contex.physical_device = physical_device;
				vk_contex.msaasample_size = GetMaxUsableSampleCount(vk_contex);
				break;
			}
		}
		if (vk_contex.physical_device == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable CPU!");
		}
	}

};