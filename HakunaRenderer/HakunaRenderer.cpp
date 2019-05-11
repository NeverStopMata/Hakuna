#include "HakunaRenderer.h"
//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>
void HakunaRenderer::InitVulkan(GLFWwindow* window)
{
	VulkanCore::CreateInstance(vk_contex_);
	SetupDebugMessenger();
	/*The window surface needs to be created right after the instance creation, because
	it can actually influence the physical device selection.*/
	VulkanCore::CreateSurface(vk_contex_, window);
	VulkanCore::PickPhysicalDevice(vk_contex_);
	VulkanCore::CreateLogicalDevice(vk_contex_);
	CreateSwapChain();
	CreateImageViewsForSwapChain();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateCommandPools();
	CreateColorResources();
	CreateDepthResources();
	CreateFramebuffers();

	
	texture_mgr_.CreateTextureImage(this->vk_contex_,"textures/chalet.jpg", diffuse_texture_);
	//CreateTextureImage();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffers();
	CreateSyncObjects();
}
//
//void HakunaRenderer::CreateInstance() {
//	if (enableValidationLayers && !checkValidationLayerSupport()) {
//		throw std::runtime_error("validation layers requested, but not available!");
//	}
//	VkApplicationInfo appInfo = {};
//	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
//	appInfo.pApplicationName = "Hello Triangle";
//	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
//	appInfo.pEngineName = "No Engine";
//	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
//	appInfo.apiVersion = VK_API_VERSION_1_0;
//	VkInstanceCreateInfo instanceCreateInfo = {};
//	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
//	instanceCreateInfo.pApplicationInfo = &appInfo;
//	auto extensions = getRequiredExtensions();
//	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
//	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
//	if (enableValidationLayers) {
//		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
//		instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
//	}
//	else
//	{
//		instanceCreateInfo.enabledLayerCount = 0;
//	}
//	if (vkCreateInstance(&instanceCreateInfo, nullptr, &vk_contex_.vk_instance) != VK_SUCCESS) {
//		throw std::runtime_error("failed to create instance!");
//	}
//}

void HakunaRenderer::InitWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window_ = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hakuna", nullptr, nullptr);
	glfwSetWindowUserPointer(window_, this);
	glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
}




//bool HakunaRenderer::checkValidationLayerSupport() {
//	uint32_t layerCount;
//	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
//	std::vector<VkLayerProperties> availableLayers(layerCount);
//	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
//	for (const char* layerName : validationLayers) {
//		bool layerFound = false;
//		for (const auto& layerProperties : availableLayers) {
//			if (strcmp(layerName, layerProperties.layerName) == 0) {
//				layerFound = true;
//				break;
//			}
//		}
//		if (!layerFound) {
//			return false;
//		}
//	}
//	return true;
//}

//std::vector<const char*> HakunaRenderer::getRequiredExtensions() {
//	uint32_t glfwExtensionCount = 0;
//	const char** glfwExtensions;
//	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
//	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
//	if (enableValidationLayers) {
//		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//	}
//	return extensions;
//}

void HakunaRenderer::SetupDebugMessenger() {
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT messagerCreateInfo = {};
	messagerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messagerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messagerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messagerCreateInfo.pfnUserCallback = debugCallback;
	messagerCreateInfo.pUserData = this;
	if (CreateDebugUtilsMessengerEXT(vk_contex_.vk_instance, &messagerCreateInfo, nullptr, &debug_messenger_) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

//void HakunaRenderer::CreateSurface() {
//	if (glfwCreateWindowSurface(vk_contex_.vk_instance, window_, nullptr, &surface_) != VK_SUCCESS) {
//		throw std::runtime_error("failed to create window surface!");
//	}
//}
//void HakunaRenderer::PickPhysicalDevice() {
//	uint32_t deviceCount = 0;
//	vkEnumeratePhysicalDevices(vk_contex_.vk_instance, &deviceCount, nullptr);
//	if (deviceCount == 0) {
//		throw std::runtime_error("failed to find GPUs with Vulkansupport!");
//	}
//	std::vector<VkPhysicalDevice> devices(deviceCount);
//	vkEnumeratePhysicalDevices(vk_contex_.vk_instance, &deviceCount, devices.data());
//	for (const auto& device : devices) {
//		if (isDeviceSuitable(device)) {
//			vk_contex_.physical_device = device;
//			vk_contex_.msaasample_size = getMaxUsableSampleCount();
//			break;
//		}
//	}
//	if (vk_contex_.physical_device == VK_NULL_HANDLE) {
//		throw std::runtime_error("failed to find a suitable CPU!");
//	}
//}

//bool HakunaRenderer::isDeviceSuitable(VkPhysicalDevice physicalDevice) {
//	VulkanCore::QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
//	bool extensionsSupported = checkDeviceExtensionsSupport(physicalDevice);
//	bool swapChainAdequate = false;
//	if (extensionsSupported) {
//		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
//		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
//	}
//	bool ret = indices.IsComplete() && extensionsSupported&& swapChainAdequate;
//	if (ret == true) {
//		this->vk_contex_.queue_family_indices = indices;
//	}
//
//	
//	return indices.IsComplete() && extensionsSupported&& swapChainAdequate;
//}
bool HakunaRenderer::checkDeviceExtensionsSupport(VkPhysicalDevice device)
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

//VulkanCore::QueueFamilyIndices HakunaRenderer::findQueueFamilies(VkPhysicalDevice device)
//{
//	VulkanCore::QueueFamilyIndices indices;
//	uint32_t queueFamilyCount = 0;
//	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
//	std::vector<VkQueueFamilyProperties> queueFamilies;
//	if (queueFamilyCount != 0) {
//		queueFamilies.resize(queueFamilyCount);
//		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
//	}
//	int i = 0;
//	for (const auto& queueFamily : queueFamilies) {
//		//find the queue family that surpports Graphic cmd
//		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
//			indices.graphicsFamily = i;
//		}
//		//find the queue family that surpports presentation(render to a rt)
//		VkBool32 presentSupport = false;
//		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
//		if (queueFamily.queueCount > 0 && presentSupport) {
//			indices.presentFamily = i;
//		}
//
//		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
//			indices.transferFamily = i;
//		}
//		if (indices.IsComplete()) {
//			break;
//		}
//		i++;
//	}
//	//std::cout << indices.graphicsFamily << " " << indices.transferFamily << " " << indices.presentFamily << std::endl;;
//				
//	return indices;
//}

//void HakunaRenderer::CreateLogicalDevice()
//{
//	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
//
//	//If the queue families are the same, then we only need to pass its index once.
//	std::set<int> uniqueQueueFamilies = { vk_contex_.queue_family_indices.graphicsFamily, vk_contex_.queue_family_indices.presentFamily,vk_contex_.queue_family_indices.transferFamily };
//	float queuePriority = 1.0f;
//	for (int queueFamily : uniqueQueueFamilies) {
//		VkDeviceQueueCreateInfo queueCreateInfo = {};
//		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
//		queueCreateInfo.queueFamilyIndex = queueFamily;
//		queueCreateInfo.queueCount = 1;
//		queueCreateInfo.pQueuePriorities = &queuePriority;
//		queueCreateInfos.push_back(queueCreateInfo);
//	}
//	VkPhysicalDeviceFeatures deviceFeatures = {};//no need for any specific feature
//	VkPhysicalDeviceFeatures supportedFeatures;
//	vkGetPhysicalDeviceFeatures(vk_contex_.physical_device, &supportedFeatures);
//	deviceFeatures.samplerAnisotropy = supportedFeatures.samplerAnisotropy;
//	deviceFeatures.sampleRateShading = VK_TRUE; // enable sample shading feature for the device
//	VkDeviceCreateInfo deviceCreateInfo = {};
//	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
//	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
//	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
//	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
//	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
//	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
//	if (enableValidationLayers) {
//		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
//		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
//	}
//	else {
//		deviceCreateInfo.enabledLayerCount = 0;
//	}
//	if (vkCreateDevice(vk_contex_.physical_device, &deviceCreateInfo, nullptr, &vk_contex_.logical_device) != VK_SUCCESS) {
//		throw std::runtime_error("failed to create logical device!");
//	}
//	vkGetDeviceQueue(vk_contex_.logical_device, static_cast<uint32_t>(vk_contex_.queue_family_indices.graphicsFamily), 0, &vk_contex_.graphics_queue);
//	vkGetDeviceQueue(vk_contex_.logical_device, static_cast<uint32_t>(vk_contex_.queue_family_indices.presentFamily), 0, &vk_contex_.present_queue);
//	vkGetDeviceQueue(vk_contex_.logical_device, static_cast<uint32_t>(vk_contex_.queue_family_indices.transferFamily), 0, &vk_contex_.transfer_queue);
//}

//SwapChainSupportDetails HakunaRenderer::querySwapChainSupport(VkPhysicalDevice device) {
//	SwapChainSupportDetails details;
//	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);
//	uint32_t formatCount;
//	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);
//	if (formatCount != 0) {
//		details.formats.resize(formatCount);
//		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
//	}
//
//	uint32_t presentModeCount;
//	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);
//	if (presentModeCount != 0) {
//		details.presentModes.resize(presentModeCount);
//		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, details.presentModes.data());
//	}
//	return details;
//}

//Each VkSurfaceFormatKHR entry contains a format and a colorSpace member.The format member specifies the color channels and types.
VkSurfaceFormatKHR HakunaRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	/*The best case scenario is that the surface has no preferred format, which Vulkan
		indicates by only returning one VkSurfaceFormatKHR entry which has its format
		member set to VK_FORMAT_UNDEFINED*/
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	/*If we’re not free to choose any format, then we’ll go through the list and see if
		the preferred combination is available :*/
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	//If that also fails then we just pick the first one
	return availableFormats[0];
}

VkPresentModeKHR HakunaRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			bestMode = availablePresentMode;
		}
	}
	return bestMode;
}
//Choose the best revolution of swap chain
//Vulkan tells us to match the resolution of the window by setting
//the width and height in the currentExtent member.However, some window
//managers do allow us to differ here and this is indicated by setting the width and
//height in currentExtent to a special value : the maximum value of uint32_t.
VkExtent2D HakunaRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window_, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
		return actualExtent;
	} 
}

void HakunaRenderer::CreateSwapChain() {
	VulkanCore::SwapChainSupportDetails swapChainSupport = VulkanCore::QuerySwapChainSupport(vk_contex_.physical_device,vk_contex_.surface);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	} 

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = vk_contex_.surface;

	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = extent;
	//The imageArrayLayers specifies the amount of layers each image consists of.
	//	This is always 1 unless you are developing a stereoscopic 3D application
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t needQueueFamilyIndices[] = {static_cast<uint32_t>(this->vk_contex_.queue_family_indices.graphicsFamily), static_cast<uint32_t>(this->vk_contex_.queue_family_indices.presentFamily) };

	if (this->vk_contex_.queue_family_indices.graphicsFamily != this->vk_contex_.queue_family_indices.presentFamily) {
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = needQueueFamilyIndices;
	}
	else {
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	/*To specify that you do not want
		any transformation, simply specify the current transformation.*/
	swapchainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	//You’ll almost always want to
	//	simply ignore the alpha channel, hence VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR.
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;

	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(vk_contex_.logical_device, &swapchainCreateInfo, nullptr, &swapchain_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(vk_contex_.logical_device, swapchain_, &imageCount, nullptr);
	swapchain_images_.resize(imageCount);
	vkGetSwapchainImagesKHR(vk_contex_.logical_device, swapchain_, &imageCount, swapchain_images_.data());

	swapchain_image_format_ = surfaceFormat.format;
	swapchain_extent_ = extent;
}

void HakunaRenderer::CreateImageViewsForSwapChain(){

	swapchain_image_views_.resize(swapchain_images_.size());
	for (size_t i = 0; i < swapchain_images_.size(); i++) {
		swapchain_image_views_[i] = VulkanCore::CreateImageView(vk_contex_,swapchain_images_[i], swapchain_image_format_, VK_IMAGE_ASPECT_COLOR_BIT,1);
	}
}
void HakunaRenderer::LoadPersistentArtResource() {
	vertex_data_mgr_ = std::make_unique<VertexDataManager>();
	vertex_data_mgr_->LoadModelFromFile("models/gun.obj");
}
void HakunaRenderer::CreateGraphicsPipeline() {
	auto vertShaderCode = ShaderManager::readFile("shaders/vert.spv");
	auto fragShaderCode = ShaderManager::readFile("shaders/frag.spv");
	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	/*vertShaderStageInfo.pSpecializationInfo = ...*/
	/*There is one more(optional) member, pSpecializationInfo,
	which we won't be using here, but is worth discussing.
	It allows you to specify values for shader constants.
	You can use a single shader module where its behavior can be configured at pipeline creation 
	by specifying different values for the constants used in it. 
	This is more efficient than configuring the shader using variables at render time,
	because the compiler can do optimizations like eliminating if statements that depend on these values.*/

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	//用于tranform
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapchain_extent_.width;
	viewport.height = (float)swapchain_extent_.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	//用于裁剪
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchain_extent_;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;//point line fragment
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;//used for shadow map

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
	multisampling.minSampleShading = .2f; // min fraction for sample shading; closer to one is smoother
	multisampling.rasterizationSamples = vk_contex_.msaasample_size;
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptor_set_layout_;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(vk_contex_.logical_device, &pipelineLayoutInfo, nullptr, &pipeline_layout_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	/*referencing the array of VkPipelineShaderStageCreateInfo structs.*/
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	/*the fixed - function stage.*/
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pDynamicState = nullptr; // Optional

	pipelineInfo.layout = pipeline_layout_;

	pipelineInfo.renderPass = renderpass_;
	pipelineInfo.subpass = 0;

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(vk_contex_.logical_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics_pipeline_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
	vkDestroyShaderModule(vk_contex_.logical_device, fragShaderModule, nullptr);
	vkDestroyShaderModule(vk_contex_.logical_device, vertShaderModule, nullptr);
}

VkShaderModule HakunaRenderer::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(vk_contex_.logical_device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
}

void HakunaRenderer::CreateRenderPass() {
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchain_image_format_;
	/*The loadOp and storeOp apply to color and depth data, and stencilLoadOp / stencilStoreOp apply to stencil data.*/
	colorAttachment.samples = vk_contex_.msaasample_size;//no multi-sample
	/*The loadOp and storeOp determine what to do with the data in the attachment before rendering and after rendering*/
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;// we don't care what previous layout the image was in.
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;//attachment 
	/*index The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!*/
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = vk_contex_.msaasample_size;
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
	colorAttachmentResolve.format = swapchain_image_format_;
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
	if (vkCreateRenderPass(vk_contex_.logical_device, &renderPassInfo, nullptr, &renderpass_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void HakunaRenderer::CreateFramebuffers() {
	swapchain_framebuffers_.resize(swapchain_image_views_.size());
	for (size_t i = 0; i < swapchain_image_views_.size(); i++) {
		std::array<VkImageView, 3> attachments = {
/*The color attachment differs for every swap chain image, 
but the same depth image can be used by all of them 
because only a single subpass is running at the same time due to our semaphores.*/
			color_image_view_,
			depth_image_view_,
			swapchain_image_views_[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderpass_;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapchain_extent_.width;
		framebufferInfo.height = swapchain_extent_.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(vk_contex_.logical_device, &framebufferInfo, nullptr, &swapchain_framebuffers_[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void HakunaRenderer::CreateCommandPools() {
	VkCommandPoolCreateInfo graphicPoolInfo = {};
	graphicPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	graphicPoolInfo.queueFamilyIndex = static_cast<uint32_t>(vk_contex_.queue_family_indices.graphicsFamily);
	graphicPoolInfo.flags = 0; // Optional
	if (vkCreateCommandPool(vk_contex_.logical_device, &graphicPoolInfo, nullptr, &vk_contex_.graphic_command_pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphic command pool!");
	}

	VkCommandPoolCreateInfo transferPoolInfo = {};
	transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferPoolInfo.queueFamilyIndex = static_cast<uint32_t>(vk_contex_.queue_family_indices.transferFamily);
	transferPoolInfo.flags = 0; // Optional
	if (vkCreateCommandPool(vk_contex_.logical_device, &transferPoolInfo, nullptr, &vk_contex_.transfer_command_pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create transfer command pool!");
	}
}

void HakunaRenderer::CreateCommandBuffers() {
	commandBuffers.resize(swapchain_framebuffers_.size());
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = vk_contex_.graphic_command_pool;
	//We won't make use of the secondary command buffer functionality here,
	//but you can imagine that it's helpful to reuse common operations from primary command buffers.
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
	if (vkAllocateCommandBuffers(vk_contex_.logical_device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		//The command buffer can be resubmitted while it is also already pending execution.
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional
		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderpass_;
		renderPassInfo.framebuffer = swapchain_framebuffers_[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchain_extent_;
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		/*The range of depths in the depth buffer is 0.0 to 1.0 in Vulkan, where 1.0 lies at the far view plane and 0.0 at the near view plane. */
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);
		VkBuffer vertexBuffers[] = { vertex_buffer_ };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], index_buffer_, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(
			commandBuffers[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			pipeline_layout_, 0, 1, &descriptor_sets_[i], 0, nullptr);
		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(vertex_data_mgr_->indices_.size()), 1, 0, 0, 0);
		vkCmdEndRenderPass(commandBuffers[i]);
		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}
/*
1.Acquire an image from the swap chain
2.Execute the command buffer with that image as attachment in the framebuffer
3.Return the image to the swap chain for presentation
Each of these events is set in motion using a single function call, but they are executed asynchronously.
*/
void HakunaRenderer::drawFrame(){
	//only after the execution of the cmd buffer with the same frame index has been finished(coresponding fence beging signaled),
	//the function will go on.
	vkWaitForFences(vk_contex_.logical_device, 1, &in_flight_fences_[current_frame_], VK_TRUE, std::numeric_limits<uint64_t>::max());
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(vk_contex_.logical_device, swapchain_, std::numeric_limits<uint64_t>::max(), image_available_semaphores_[current_frame_], VK_NULL_HANDLE, &imageIndex);

	/*VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible with the surface and can no longer be used for rendering. 
	Usually happens after a window resize.
      VK_SUBOPTIMAL_KHR: The swap chain can still be used to successfully present to the surface, but the surface properties are no longer matched exactly.*/
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		ReCreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	UpdateUniformBuffer(imageIndex);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	//需要等待信号量的阶段和其等待的信号量是在两个数组里，一一对应
	VkSemaphore waitSemaphores[] = { image_available_semaphores_[current_frame_] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };//需要等待的阶段 颜色输出到attachment才需要等待imageAvailable,vert shader可以先开始
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
	////The signalSemaphoreCount and pSignalSemaphores parameters specify which semaphores to signal once the command buffer(s) have finished execution.
	VkSemaphore signalSemaphores[] = { render_finished_semaphores_[current_frame_] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// fence wont be reset automatically while semaphores can be reset as soon as it is caught.
	vkResetFences(vk_contex_.logical_device, 1, &in_flight_fences_[current_frame_]);

	//after finishing the execution of the submitted cmd, inFlightFences[i] will be signaled.
	if (vkQueueSubmit(vk_contex_.graphics_queue, 1, &submitInfo, in_flight_fences_[current_frame_]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swapChains[] = { swapchain_ };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;//for check whether each image in the swap chain is presented successfully.
	result = vkQueuePresentKHR(vk_contex_.present_queue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized_) {
		framebuffer_resized_ = false;
		ReCreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
	current_frame_ = (current_frame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

/*
To perform GPU - GPU synchronization, Vulkan offers a second type of synchronization primitive called Semaphores.
To perform CPU - GPU synchronization, Vulkan offers a second type of synchronization primitive called Fences.
*/
void HakunaRenderer::CreateSyncObjects() {
	image_available_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
	render_finished_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
	in_flight_fences_.resize(MAX_FRAMES_IN_FLIGHT);
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(vk_contex_.logical_device, &semaphoreInfo, nullptr,&image_available_semaphores_[i]) != VK_SUCCESS ||
			vkCreateSemaphore(vk_contex_.logical_device, &semaphoreInfo, nullptr,&render_finished_semaphores_[i]) != VK_SUCCESS ||
			vkCreateFence(vk_contex_.logical_device, &fenceInfo, nullptr,&in_flight_fences_[i]) != VK_SUCCESS){
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

void HakunaRenderer::CreateVertexBuffer() {
	std::vector<uint32_t> stadingBufferQueueFamilyIndices{ static_cast<uint32_t>(vk_contex_.queue_family_indices.transferFamily)};
	std::vector<uint32_t> deviceLocalBufferQueueFamilyIndices{ 
		static_cast<uint32_t>(vk_contex_.queue_family_indices.transferFamily),
		static_cast<uint32_t>(vk_contex_.queue_family_indices.graphicsFamily)};
	VkDeviceSize bufferSize = sizeof(vertex_data_mgr_->vertices_[0]) * vertex_data_mgr_->vertices_.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VulkanCore::CreateBuffer(vk_contex_, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer, 
		stagingBufferMemory,
		1, nullptr);

	void* data;
	vkMapMemory(vk_contex_.logical_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertex_data_mgr_->vertices_.data(), (size_t)bufferSize);
	vkUnmapMemory(vk_contex_.logical_device, stagingBufferMemory);

	VulkanCore::CreateBuffer(vk_contex_,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertex_buffer_, 
		vertex_buffer_memory_,
		2,
		deviceLocalBufferQueueFamilyIndices.data());
	
	CopyBuffer(stagingBuffer, vertex_buffer_, bufferSize);

	vkDestroyBuffer(vk_contex_.logical_device, stagingBuffer, nullptr);
	vkFreeMemory(vk_contex_.logical_device, stagingBufferMemory, nullptr);
}

void HakunaRenderer::CreateIndexBuffer()
{
	std::vector<uint32_t> deviceLocalBufferQueueFamilyIndices{
		static_cast<uint32_t>(vk_contex_.queue_family_indices.transferFamily),
		static_cast<uint32_t>(vk_contex_.queue_family_indices.graphicsFamily) };
	VkDeviceSize bufferSize = sizeof(vertex_data_mgr_->indices_[0]) * vertex_data_mgr_->indices_.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VulkanCore::CreateBuffer(vk_contex_,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, 
		stagingBufferMemory,
		1,
		nullptr);

	void* data;
	vkMapMemory(vk_contex_.logical_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertex_data_mgr_->indices_.data(), (size_t)bufferSize);
	vkUnmapMemory(vk_contex_.logical_device, stagingBufferMemory);

	VulkanCore::CreateBuffer(vk_contex_,
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		index_buffer_, 
		index_buffer_memory_,
		2,
		deviceLocalBufferQueueFamilyIndices.data());

	CopyBuffer(stagingBuffer, index_buffer_, bufferSize);

	vkDestroyBuffer(vk_contex_.logical_device, stagingBuffer, nullptr);
	vkFreeMemory(vk_contex_.logical_device, stagingBufferMemory, nullptr);
}

void HakunaRenderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(static_cast<uint32_t>(vk_contex_.queue_family_indices.transferFamily));

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer, static_cast<uint32_t>(vk_contex_.queue_family_indices.transferFamily));
}

void HakunaRenderer::ReCreateSwapChain() {
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window_, &width, &height);
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(vk_contex_.logical_device);
	cleanupSwapChain();
	CreateSwapChain();
	CreateImageViewsForSwapChain();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateColorResources();
	CreateDepthResources();
	CreateFramebuffers();
	CreateCommandBuffers();
}
void HakunaRenderer::cleanupSwapChain() {
	for (size_t i = 0; i < swapchain_framebuffers_.size(); i++) {
		vkDestroyFramebuffer(vk_contex_.logical_device, swapchain_framebuffers_[i], nullptr);
	}
	vkDestroyImageView(vk_contex_.logical_device, color_image_view_, nullptr);
	vkDestroyImage(vk_contex_.logical_device, color_image_, nullptr);
	vkFreeMemory(vk_contex_.logical_device, color_image_memory_, nullptr);
	vkDestroyImageView(vk_contex_.logical_device, depth_image_view_, nullptr);
	vkDestroyImage(vk_contex_.logical_device, depth_image_, nullptr);
	vkFreeMemory(vk_contex_.logical_device, depth_image_memory_, nullptr);
	/* This way we can reuse the existing pool to allocate the new command buffers*/
	vkFreeCommandBuffers(vk_contex_.logical_device, vk_contex_.graphic_command_pool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	vkDestroyPipeline(vk_contex_.logical_device, graphics_pipeline_, nullptr);
	vkDestroyPipelineLayout(vk_contex_.logical_device, pipeline_layout_, nullptr);
	vkDestroyRenderPass(vk_contex_.logical_device, renderpass_, nullptr);

	for (size_t i = 0; i < swapchain_image_views_.size(); i++) {
		vkDestroyImageView(vk_contex_.logical_device, swapchain_image_views_[i], nullptr);
	}

	vkDestroySwapchainKHR(vk_contex_.logical_device, swapchain_, nullptr);
}

void HakunaRenderer::CreateDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	//descriptorCount specifies the number of values in the array. 
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(vk_contex_.logical_device, &layoutInfo, nullptr, &descriptor_set_layout_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void HakunaRenderer::CreateUniformBuffers() {
	std::vector<uint32_t> uboQueueFamilyIndices{
		static_cast<uint32_t>(vk_contex_.queue_family_indices.transferFamily),
		static_cast<uint32_t>(vk_contex_.queue_family_indices.graphicsFamily) };
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	uniform_buffers_.resize(swapchain_images_.size());
	uniform_buffers_memory_.resize(swapchain_images_.size());

	for (size_t i = 0; i < swapchain_images_.size(); i++) {
		VulkanCore::CreateBuffer(vk_contex_,
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniform_buffers_[i], 
			uniform_buffers_memory_[i],
			1, nullptr);
	}
}

void HakunaRenderer::UpdateUniformBuffer(uint32_t currentImage) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(
		glm::mat4(1.0f), 
		0.1f * time * glm::radians(90.0f), 
		glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.view = cam_->GetViewMatrix();
	ubo.proj = cam_->GetProjMatrix();
	//GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. 
	ubo.proj[1][1] *= -1;
	void* data;
	vkMapMemory(vk_contex_.logical_device, uniform_buffers_memory_[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(vk_contex_.logical_device, uniform_buffers_memory_[currentImage]);
}
void HakunaRenderer::Cleanup()
{
	cleanupSwapChain();
	vkDestroyDescriptorSetLayout(vk_contex_.logical_device, descriptor_set_layout_, nullptr);
	for (size_t i = 0; i < swapchain_images_.size(); i++) {
		vkDestroyBuffer(vk_contex_.logical_device, uniform_buffers_[i], nullptr);
		vkFreeMemory(vk_contex_.logical_device, uniform_buffers_memory_[i], nullptr);
	}
	vkDestroyBuffer(vk_contex_.logical_device, vertex_buffer_, nullptr);
	vkFreeMemory(vk_contex_.logical_device, vertex_buffer_memory_, nullptr);
	vkDestroyBuffer(vk_contex_.logical_device, index_buffer_, nullptr);
	vkFreeMemory(vk_contex_.logical_device, index_buffer_memory_, nullptr);
	vkDestroySampler(vk_contex_.logical_device, diffuse_texture_.texture_sampler, nullptr);
	vkDestroyImageView(vk_contex_.logical_device, diffuse_texture_.texture_image_view, nullptr);
	vkDestroyImage(vk_contex_.logical_device, diffuse_texture_.texture_image, nullptr);
	vkFreeMemory(vk_contex_.logical_device, diffuse_texture_.texture_image_memory, nullptr);
	vkDestroyDescriptorPool(vk_contex_.logical_device, descriptor_pool_, nullptr);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(vk_contex_.logical_device, render_finished_semaphores_[i], nullptr);
		vkDestroySemaphore(vk_contex_.logical_device, image_available_semaphores_[i], nullptr);
		vkDestroyFence(vk_contex_.logical_device, in_flight_fences_[i], nullptr);
	}

	vkDestroyCommandPool(vk_contex_.logical_device, vk_contex_.graphic_command_pool, nullptr);
	vkDestroyCommandPool(vk_contex_.logical_device, vk_contex_.transfer_command_pool, nullptr);
	vkDestroyDevice(vk_contex_.logical_device, nullptr);

	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(vk_contex_.vk_instance, debug_messenger_, nullptr);
	}
	vkDestroySurfaceKHR(vk_contex_.vk_instance, vk_contex_.surface, nullptr);//Make sure that the surface is destroyed before the instance.
	vkDestroyInstance(vk_contex_.vk_instance, nullptr);
	glfwDestroyWindow(window_);
	glfwTerminate();
}

void HakunaRenderer::CreateDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swapchain_images_.size());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(swapchain_images_.size());
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapchain_images_.size());
	if (vkCreateDescriptorPool(vk_contex_.logical_device, &poolInfo, nullptr, &descriptor_pool_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}
void HakunaRenderer::CreateDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(swapchain_images_.size(), descriptor_set_layout_);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptor_pool_;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchain_images_.size());
	allocInfo.pSetLayouts = layouts.data();
	descriptor_sets_.resize(swapchain_images_.size());
	if (vkAllocateDescriptorSets(vk_contex_.logical_device, &allocInfo, descriptor_sets_.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
	for (size_t i = 0; i < swapchain_images_.size(); i++) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniform_buffers_[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = diffuse_texture_.texture_image_view;
		imageInfo.sampler = diffuse_texture_.texture_sampler;
		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptor_sets_[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptor_sets_[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(vk_contex_.logical_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

}

void HakunaRenderer::CreateDepthResources() {
	VkFormat depthFormat = findDepthFormat();
	VulkanCore::CreateImage(
		vk_contex_,
		swapchain_extent_.width,
		swapchain_extent_.height, 
		1,
		vk_contex_.msaasample_size,
		depthFormat, 
		VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		depth_image_, 
		depth_image_memory_
	);
	depth_image_view_ = VulkanCore::CreateImageView(vk_contex_,depth_image_, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT,1);
	TransitionImageLayout(depth_image_, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,1);
}

void HakunaRenderer::CreateColorResources() {
	VkFormat colorFormat = swapchain_image_format_;

	VulkanCore::CreateImage(
		vk_contex_,
		swapchain_extent_.width, 
		swapchain_extent_.height,
		1, 
		vk_contex_.msaasample_size,
		colorFormat, 
		VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | 
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		color_image_, 
		color_image_memory_);
	color_image_view_ = VulkanCore::CreateImageView(vk_contex_,color_image_, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	TransitionImageLayout(color_image_, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
}

VkFormat HakunaRenderer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(vk_contex_.physical_device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat HakunaRenderer::findDepthFormat() {
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

void HakunaRenderer::GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {

	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(vk_contex_.physical_device, imageFormat, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(static_cast<uint32_t>(vk_contex_.queue_family_indices.graphicsFamily));

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

	endSingleTimeCommands(commandBuffer,static_cast<uint32_t>(vk_contex_.queue_family_indices.graphicsFamily));
}
void HakunaRenderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(static_cast<uint32_t>(vk_contex_.queue_family_indices.graphicsFamily));

	VkImageMemoryBarrier barrier = {};
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (VulkanCore::HasStencilComponent(format)) {
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
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	} else {
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
	endSingleTimeCommands(commandBuffer, static_cast<uint32_t>(vk_contex_.queue_family_indices.graphicsFamily));
}


void HakunaRenderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(static_cast<uint32_t>(vk_contex_.queue_family_indices.transferFamily));
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
	endSingleTimeCommands(commandBuffer, static_cast<uint32_t>(vk_contex_.queue_family_indices.transferFamily));
}

VkCommandBuffer HakunaRenderer::beginSingleTimeCommands(uint32_t queueFamilyIndex) {
	VkCommandPool commandPool;
	if (queueFamilyIndex == vk_contex_.queue_family_indices.transferFamily) {
		commandPool = vk_contex_.transfer_command_pool;
	}
	else if (queueFamilyIndex == vk_contex_.queue_family_indices.graphicsFamily) {
		commandPool = vk_contex_.graphic_command_pool;
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
	vkAllocateCommandBuffers(vk_contex_.logical_device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void HakunaRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer, uint32_t queueFamilyIndex) {
	VkCommandPool commandPool;
	VkQueue queue;
	if (queueFamilyIndex == vk_contex_.queue_family_indices.transferFamily) {
		commandPool = vk_contex_.transfer_command_pool;
		queue = vk_contex_.transfer_queue;
	}
	else if (queueFamilyIndex == vk_contex_.queue_family_indices.graphicsFamily) {
		commandPool = vk_contex_.graphic_command_pool;
		queue = vk_contex_.graphics_queue;
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

	vkFreeCommandBuffers(vk_contex_.logical_device, commandPool, 1, &commandBuffer);
}

VkSampleCountFlagBits HakunaRenderer::getMaxUsableSampleCount(){
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(vk_contex_.physical_device, &physicalDeviceProperties);

	VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

