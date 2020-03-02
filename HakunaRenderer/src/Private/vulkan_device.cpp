#include "vulkan_device.h"
#include "vulkan_utility.h"
VkSampleCountFlagBits VulkanDevice::GetMaxUsableSampleCount()
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physical_device, &physicalDeviceProperties);

	VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}
VulkanDevice::QueueFamilyIndices VulkanDevice::GetQueueFamilies(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
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

		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			indices.computeFamily = i;
		}
		if (indices.IsComplete()) {
			break;
		}
		i++;
	}
	//std::cout << indices.graphicsFamily << " " << indices.transferFamily << " " << indices.presentFamily << std::endl;;

	return indices;
}

bool VulkanDevice::CheckDeviceExtensionsSupport(VkPhysicalDevice device)
{
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

SwapChainSupportDetails VulkanDevice::QuerySwapChainSupport(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
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

VkResult VulkanDevice::CreateBuffer(
		VkBufferUsageFlags usageFlags, 
		VkMemoryPropertyFlags memoryPropertyFlags,
		VulkanBuffer* buffer, 
		VkDeviceSize size, 
		int sharingQueueFamilyCount,
		const uint32_t* psharingQueueFamilyIndices,
		void* data)const
{
	buffer->device = logical_device;
	// Create the buffer handle
	VkBufferCreateInfo bufferCreateInfo = VKInitializers::bufferCreateInfo(usageFlags, size, sharingQueueFamilyCount, psharingQueueFamilyIndices);
	VK_CHECK_RESULT(vkCreateBuffer(logical_device, &bufferCreateInfo, nullptr, &buffer->buffer));

	// Create the memory backing up the buffer handle
	VkMemoryRequirements memReqs;
	VkMemoryAllocateInfo memAlloc = VKInitializers::memoryAllocateInfo();
	vkGetBufferMemoryRequirements(logical_device, buffer->buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	// Find a memory type index that fits the properties of the buffer
	memAlloc.memoryTypeIndex = GetMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
	VK_CHECK_RESULT(vkAllocateMemory(logical_device, &memAlloc, nullptr, &buffer->memory));

	buffer->alignment = memReqs.alignment;
	buffer->size = memAlloc.allocationSize;
	buffer->usageFlags = usageFlags;
	buffer->memoryPropertyFlags = memoryPropertyFlags;

	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
	if (data != nullptr)
	{
		VK_CHECK_RESULT(buffer->map());
		memcpy(buffer->mapped, data, size);
		if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			buffer->flush();

		buffer->unmap();
	}

	// Initialize a default descriptor that covers the whole buffer size
	buffer->setupDescriptor();

	// Attach the memory to the buffer object
	return buffer->bind();
}

VkResult VulkanDevice::CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, void* data)
{
	// Create the buffer handle
	VkBufferCreateInfo bufferCreateInfo = VKInitializers::bufferCreateInfo(usageFlags, size);
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK_RESULT(vkCreateBuffer(logical_device, &bufferCreateInfo, nullptr, buffer));

	// Create the memory backing up the buffer handle
	VkMemoryRequirements memReqs;
	VkMemoryAllocateInfo memAlloc = VKInitializers::memoryAllocateInfo();
	vkGetBufferMemoryRequirements(logical_device, *buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	// Find a memory type index that fits the properties of the buffer
	memAlloc.memoryTypeIndex = GetMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
	VK_CHECK_RESULT(vkAllocateMemory(logical_device, &memAlloc, nullptr, memory));

	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
	if (data != nullptr)
	{
		void* mapped;
		VK_CHECK_RESULT(vkMapMemory(logical_device, *memory, 0, size, 0, &mapped));
		memcpy(mapped, data, size);
		// If host coherency hasn't been requested, do a manual flush to make writes visible
		if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
		{
			VkMappedMemoryRange mappedRange = VKInitializers::mappedMemoryRange();
			mappedRange.memory = *memory;
			mappedRange.offset = 0;
			mappedRange.size = size;
			vkFlushMappedMemoryRanges(logical_device, 1, &mappedRange);
		}
		vkUnmapMemory(logical_device, *memory);
	}

	// Attach the memory to the buffer object
	VK_CHECK_RESULT(vkBindBufferMemory(logical_device, *buffer, *memory, 0));

	return VK_SUCCESS;
}

bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices = GetQueueFamilies(physical_device, surface);
	bool extensionsSupported = CheckDeviceExtensionsSupport(physical_device);
	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physical_device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}
	bool ret = indices.IsComplete() && extensionsSupported && swapChainAdequate;
	if (ret == true) {
		queue_family_indices = indices;
	}
	return indices.IsComplete() && extensionsSupported && swapChainAdequate;
}
void VulkanDevice::PickPhysicalDevice(VkInstance vk_instance, VkSurfaceKHR surface)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(vk_instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkansupport!");
	}
	std::vector<VkPhysicalDevice> physical_devices(deviceCount);
	vkEnumeratePhysicalDevices(vk_instance, &deviceCount, physical_devices.data());
	for (const auto& tmp_physical_device : physical_devices) {
		if (IsDeviceSuitable(tmp_physical_device, surface)) {
			physical_device = tmp_physical_device;
			msaasample_size = GetMaxUsableSampleCount();
			break;
		}
	}
	if (physical_device == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

void VulkanDevice::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	//If the queue families are the same, then we only need to pass its index once.
	std::set<int> uniqueQueueFamilies = { queue_family_indices.graphicsFamily, queue_family_indices.presentFamily,queue_family_indices.transferFamily };
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
	vkGetPhysicalDeviceFeatures(physical_device, &supportedFeatures);
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
	if (vkCreateDevice(physical_device, &deviceCreateInfo, nullptr, &logical_device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}
	vkGetDeviceQueue(logical_device, static_cast<uint32_t>(queue_family_indices.graphicsFamily), 0, &graphics_queue);
	vkGetDeviceQueue(logical_device, static_cast<uint32_t>(queue_family_indices.presentFamily), 0, &present_queue);
	vkGetDeviceQueue(logical_device, static_cast<uint32_t>(queue_family_indices.transferFamily), 0, &transfer_queue);
}

void VulkanDevice::CreateCommandPools()
{
	VkCommandPoolCreateInfo graphicPoolInfo = {};
	graphicPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	graphicPoolInfo.queueFamilyIndex = static_cast<uint32_t>(queue_family_indices.graphicsFamily);
	graphicPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional
	if (vkCreateCommandPool(logical_device, &graphicPoolInfo, nullptr, &graphic_command_pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphic command pool!");
	}

	VkCommandPoolCreateInfo transferPoolInfo = {};
	transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferPoolInfo.queueFamilyIndex = static_cast<uint32_t>(queue_family_indices.transferFamily);
	transferPoolInfo.flags = 0; // Optional
	if (vkCreateCommandPool(logical_device, &transferPoolInfo, nullptr, &transfer_command_pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create transfer command pool!");
	}
}

uint32_t VulkanDevice::GetMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	return -1;
}

void VulkanDevice::BeginSingleTimeCommands(uint32_t queueFamilyIndex, VkCommandBuffer& commandBuffer)const
{
	VkCommandPool commandPool;
	if (queueFamilyIndex == queue_family_indices.transferFamily) {
		commandPool = transfer_command_pool;
	}
	else if (queueFamilyIndex == queue_family_indices.graphicsFamily) {
		commandPool = graphic_command_pool;
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
	vkAllocateCommandBuffers(logical_device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
}

void VulkanDevice::EndSingleTimeCommands(VkCommandBuffer& commandBuffer, uint32_t queueFamilyIndex) const
{
	VkCommandPool commandPool;
	VkQueue queue;
	if (queueFamilyIndex == queue_family_indices.transferFamily) {
		commandPool = transfer_command_pool;
		queue = transfer_queue;
	}
	else if (queueFamilyIndex == queue_family_indices.graphicsFamily) {
		commandPool = graphic_command_pool;
		queue = graphics_queue;
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
	vkFreeCommandBuffers(logical_device, commandPool, 1, &commandBuffer);
}