#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>
#include <string>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <vulkan/vulkan.h>
#include "VulkanTools.h"

typedef struct _SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;//Basic surface capabilities (min/max number of images in swap chain,	min / max width and height of images)
	std::vector<VkSurfaceFormatKHR> formats;//Surface formats (pixel format, color space)
	std::vector<VkPresentModeKHR> presentModes;
} SwapChainSupportDetails;

typedef struct _SwapChainBuffers {
	VkImage image;
	VkImageView view;
} SwapChainBuffer;

class VKSwapchain
{
private:
	VkInstance instance_;
	VkDevice device_;
	VkPhysicalDevice physicalDevice_;
	SwapChainSupportDetails swapChainSupportDetails_;
	// Function pointers
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
	PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
	PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
	PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
	PFN_vkQueuePresentKHR fpQueuePresentKHR;
public:
	VkSurfaceKHR surface_;
	VkFormat colorFormat_;
	VkColorSpaceKHR colorSpace_;
	/** @brief Handle to the current swap chain, required for recreation */
	VkSwapchainKHR swapChain_ = VK_NULL_HANDLE;
	uint32_t imageCount_;
	std::vector<VkImage> images_;
	std::vector<SwapChainBuffer> buffers_;
	VkExtent2D extent_;

private:

	void QuerySwapChainSupport(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool is_vsync);

	VkExtent2D ChooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);

public:

	void CreateSurface(GLFWwindow* windows, VkInstance vk_instance_);

	void Connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);

	void CreateSwapchain(GLFWwindow* window, bool vsync = false);

	void Cleanup();
	
};