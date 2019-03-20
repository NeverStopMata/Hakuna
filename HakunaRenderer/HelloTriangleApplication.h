#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <set>
#include <algorithm>
#include "ShaderManager.h"
const int MAX_FRAMES_IN_FLIGHT = 2;
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME//显式地设置swap chain 的扩展（虽然只要presentation的queue family存在就可以说明该物理设备支持输出到屏幕）
};
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
	int graphicsFamily = -1;//绘制需要
	int presentFamily = -1; //需要将画面输出到屏幕
	bool isComplete() {
		return graphicsFamily >= 0 && presentFamily >= 0;
	}
};//表示物理设备所支持的且需要的queue families的index.

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;//Basic surface capabilities (min/max number of images in swap chain,	min / max width and height of images)
	std::vector<VkSurfaceFormatKHR> formats;//Surface formats (pixel format, color space)
	std::vector<VkPresentModeKHR> presentModes;
};

VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance, 
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(
	VkInstance instance, 
	VkDebugUtilsMessengerEXT debugMessenger, 
	const VkAllocationCallbacks* pAllocator);
class HelloTriangleApplication
{

private:
	GLFWwindow *window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	//The logical device should be destroyed in cleanup with the vkDestroyDevice function:
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapChainFramebuffers;//with the size of swapChain image count.

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;//with the size of swapChain image count.

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	size_t currentFrame = 0;
public:
	HelloTriangleApplication();
	~HelloTriangleApplication();
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}
private:
	void initVulkan();
	void createInstance();
	void createSurface();
	void pickPhysicalDevice();
	void setupDebugMessenger();
	void initWindow();
	void mainLoop();
	void cleanup();
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionsSupport(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device); 
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	void createLogicalDevice();
	void createSwapChain();
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void createImageViews();
	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>& code); 
	void createRenderPass();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();
	void drawFrame();
	void createSyncObjects();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
		VkDebugUtilsMessageTypeFlagsEXT messageType, 
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
		void* pUserData) {

		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

};

