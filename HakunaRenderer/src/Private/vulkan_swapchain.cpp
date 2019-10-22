#include "vulkan_swapchain.h"
#include "vulkan_device.h"
void VulkanSwapchain::CreateSurface(GLFWwindow* windows, VkInstance vk_instance_)
{
	if (glfwCreateWindowSurface(vk_instance_, windows, nullptr, &(this->surface_)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}
void VulkanSwapchain::Connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{
	this->instance_ = instance;
	this->physicalDevice_ = physicalDevice;
	this->device_ = device;
	GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceSupportKHR);
	GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
	GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceFormatsKHR);
	GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfacePresentModesKHR);
	GET_DEVICE_PROC_ADDR(device, CreateSwapchainKHR);
	GET_DEVICE_PROC_ADDR(device, DestroySwapchainKHR);
	GET_DEVICE_PROC_ADDR(device, GetSwapchainImagesKHR);
	GET_DEVICE_PROC_ADDR(device, AcquireNextImageKHR);
	GET_DEVICE_PROC_ADDR(device, QueuePresentKHR);
}
void VulkanSwapchain::QuerySwapChainSupport(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &swapChainSupportDetails_.capabilities);
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		swapChainSupportDetails_.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount, swapChainSupportDetails_.formats.data());
	}
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		swapChainSupportDetails_.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &presentModeCount, swapChainSupportDetails_.presentModes.data());
	}
}
VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	/*The best case scenario is that the surface has no preferred format, which Vulkan
		indicates by only returning one VkSurfaceFormatKHR entry which has its format
		member set to VK_FORMAT_UNDEFINED*/
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	/*If we¡¯re not free to choose any format, then we¡¯ll go through the list and see if
		the preferred combination is available :*/
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	//If that also fails then we just pick the first one
	return availableFormats[0];
}
VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool is_vsync)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
	if (!is_vsync)
	{
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
			else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				bestMode = availablePresentMode;
			}
		}
	}
	return bestMode;
}
VkExtent2D VulkanSwapchain::ChooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
		return actualExtent;
	}
}
void VulkanSwapchain::Cleanup()
{
	if (swapChain_ != VK_NULL_HANDLE)
	{
		for (uint32_t i = 0; i < imageCount_; i++)
		{
			vkDestroyImageView(device_, buffers_[i].view, nullptr);
		}
	}
	if (surface_ != VK_NULL_HANDLE)
	{
		fpDestroySwapchainKHR(device_, swapChain_, nullptr);
		vkDestroySurfaceKHR(instance_, surface_, nullptr);
	}
	surface_ = VK_NULL_HANDLE;
	swapChain_ = VK_NULL_HANDLE;
}

void VulkanSwapchain::CreateSwapchain(GLFWwindow* window, const VulkanDevice& vk_device, bool vsync)
{
	QuerySwapChainSupport(physicalDevice_, surface_);
	VkSwapchainKHR oldSwapchain = swapChain_;
	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupportDetails_.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupportDetails_.presentModes, vsync);
	extent_ = ChooseSwapExtent(window, swapChainSupportDetails_.capabilities);

	imageCount_ = swapChainSupportDetails_.capabilities.minImageCount + 1;
	if (swapChainSupportDetails_.capabilities.maxImageCount > 0 && imageCount_ > swapChainSupportDetails_.capabilities.maxImageCount) {
		imageCount_ = swapChainSupportDetails_.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = this->surface_;

	swapchainCreateInfo.minImageCount = imageCount_;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = extent_;
	//The imageArrayLayers specifies the amount of layers each image consists of.
	//	This is always 1 unless you are developing a stereoscopic 3D application
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


	auto indices = vk_device.queue_family_indices;
	uint32_t needQueueFamilyIndices[] = {
		static_cast<uint32_t>(indices.graphicsFamily),
		static_cast<uint32_t>(indices.presentFamily) };

	if (indices.graphicsFamily != indices.presentFamily) {
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = needQueueFamilyIndices;
	}
	else {
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	/*To specify that you do not want
		any transformation, simply specify the current transformation.*/
	swapchainCreateInfo.preTransform = swapChainSupportDetails_.capabilities.currentTransform;
	//You¡¯ll almost always want to
	//	simply ignore the alpha channel, hence VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR.
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = oldSwapchain;
	VK_CHECK_RESULT(vkCreateSwapchainKHR(device_, &swapchainCreateInfo, nullptr, &swapChain_));
	vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount_, nullptr);
	images_.resize(imageCount_);
	vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount_, images_.data());

	colorFormat_ = surfaceFormat.format;


	/**********************************************************************/

	// If an existing swap chain is re-created, destroy the old swap chain
	// This also cleans up all the presentable images
	if (oldSwapchain != VK_NULL_HANDLE)
	{
		for (uint32_t i = 0; i < imageCount_; i++)
		{
			vkDestroyImageView(device_, buffers_[i].view, nullptr);
		}
		fpDestroySwapchainKHR(device_, oldSwapchain, nullptr);
	}
	VK_CHECK_RESULT(fpGetSwapchainImagesKHR(device_, swapChain_, &imageCount_, NULL));

	// Get the swap chain images
	images_.resize(imageCount_);
	VK_CHECK_RESULT(fpGetSwapchainImagesKHR(device_, swapChain_, &imageCount_, images_.data()));

	// Get the swap chain buffers containing the image and imageview
	buffers_.resize(imageCount_);
	for (uint32_t i = 0; i < imageCount_; i++)
	{
		VkImageViewCreateInfo colorAttachmentView = {};
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.pNext = NULL;
		colorAttachmentView.format = colorFormat_;
		colorAttachmentView.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.baseMipLevel = 0;
		colorAttachmentView.subresourceRange.levelCount = 1;
		colorAttachmentView.subresourceRange.baseArrayLayer = 0;
		colorAttachmentView.subresourceRange.layerCount = 1;
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorAttachmentView.flags = 0;

		buffers_[i].image = images_[i];

		colorAttachmentView.image = buffers_[i].image;

		VK_CHECK_RESULT(vkCreateImageView(device_, &colorAttachmentView, nullptr, &buffers_[i].view));
	}
}