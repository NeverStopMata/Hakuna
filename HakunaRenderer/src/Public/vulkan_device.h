#pragma once
#include <vector>
#include <set>
#include <stdexcept>
#include "vulkan/vulkan.h"
#include "vulkan_swapchain.h"
#include "VulkanBuffer.hpp"
#include "VulkanInitializers.hpp"
class VulkanUtility;
class VulkanDevice
{
	struct QueueFamilyIndices {
		int graphicsFamily = -1;//绘制需要
		int presentFamily = -1; //需要将画面输出到屏幕
		int transferFamily = -1;
		int computeFamily = -1;
		bool IsComplete() {
			return graphicsFamily >= 0 &&
				presentFamily >= 0 &&
				transferFamily >= 0 &&
				computeFamily >= 0;
		}
	};
public:
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
	VkSampleCountFlagBits msaasample_size;
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME//显式地设置swap chain 的扩展（虽然只要presentation的queue family存在就可以说明该物理设备支持输出到屏幕）
	};

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
	void PickPhysicalDevice(VkInstance vk_instance, VkSurfaceKHR surface);
	void CreateLogicalDevice();
	void CreateCommandPools();
	uint32_t GetMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	void BeginSingleTimeCommands(uint32_t queueFamilyIndex, VkCommandBuffer& commandBuffer) const;
	void EndSingleTimeCommands(VkCommandBuffer& commandBuffer, uint32_t queueFamilyIndex) const;

	/**
		* Create a buffer on the device
		*
		* @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
		* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
		* @param buffer Pointer to a vk::Vulkan buffer object
		* @param size Size of the buffer in byes
		* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
		*
		* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
		*/
	VkResult CreateBuffer(
		VkBufferUsageFlags usageFlags,
		VkMemoryPropertyFlags memoryPropertyFlags,
		VulkanBuffer* buffer,
		VkDeviceSize size,
		int sharingQueueFamilyCount = 1,
		const uint32_t* psharingQueueFamilyIndices = nullptr,
		void* data = nullptr)const;

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommands(static_cast<uint32_t>(queue_family_indices.transferFamily), commandBuffer);
		VkBufferCopy copyRegion = {};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
		EndSingleTimeCommands(commandBuffer, static_cast<uint32_t>(queue_family_indices.transferFamily));
	}
private:
	bool IsDeviceSuitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
	VkSampleCountFlagBits GetMaxUsableSampleCount();
	QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
	bool CheckDeviceExtensionsSupport(VkPhysicalDevice device);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
};

