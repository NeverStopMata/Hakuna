#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>



#include <chrono>
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
#include "VertexDataManager.h"
const int MAX_FRAMES_IN_FLIGHT = 2;
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const std::string TEXTURE_PATH = "textures/chalet.jpg";
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
/*
Scalars have to be aligned by N(= 4 bytes given 32 bit floats).
A vec2 must be aligned by 2N(= 8 bytes)
A vec3 or vec4 must be aligned by 4N(= 16 bytes)
A nested structure must be aligned by the base alignment of its members rounded up to a multiple of 16.
A mat4 matrix must have the same alignment as a vec4.
*/
struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct QueueFamilyIndices {
	int graphicsFamily = -1;//绘制需要
	int presentFamily = -1; //需要将画面输出到屏幕
	int transferFamily = -1;
	bool isComplete() {
		return graphicsFamily >= 0 &&
				presentFamily >= 0 &&
				transferFamily >= 0;
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
	VkQueue transferQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkImageView textureImageView;
	VkSampler textureSampler;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapChainFramebuffers;//with the size of swapChain image count.
	VkCommandPool transferCommandPool;
	VkCommandPool graphicCommandPool;

	std::vector<VkCommandBuffer> commandBuffers;//with the size of swapChain image count.
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	//used for multi-sampling
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	uint32_t mipLevels;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;


	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	size_t currentFrame = 0;
	QueueFamilyIndices queueFamilyIndices;
	VertexDataManager * pVertexDataMgr;
	/*Although many drivers and platforms trigger VK_ERROR_OUT_OF_DATE_KHR automatically after a window resize, 
	it is not guaranteed to happen. That's why we'll add some extra code to also handle resizes explicitly. */
	bool framebufferResized = false;
public:
	HelloTriangleApplication();
	~HelloTriangleApplication();
	void run() {
		initPersistentArtResource();
		std::cout << "hello" << std::endl;
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
	void cleanupSwapChain();
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionsSupport(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device); 
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	void createLogicalDevice();
	void createSwapChain();
	void recreateSwapChain();
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void createImageViews();
	void createTextureImageView();
	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>& code); 
	void createRenderPass();
	void createFramebuffers();
	void createCommandPools();
	void createCommandBuffers();
	void drawFrame();
	void createSyncObjects();
	void createVertexBuffer();
	void createIndexBuffer();
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory,
		int sharingQueueFamilyCount,
		const uint32_t* psharingQueueFamilyIndices);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void createDescriptorSetLayout();
	void createUniformBuffers();
	void updateUniformBuffer(uint32_t currentImage);
	void createDescriptorPool();
	void createDescriptorSets();
	VkImageView createImageView(VkImage image, VkFormat forma, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void createTextureImage();
	void createImage(
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
		imageMemory);
	VkCommandBuffer beginSingleTimeCommands(uint32_t queueFamilyIndex);
	void endSingleTimeCommands(VkCommandBuffer commandBuffer, uint32_t queueFamilyIndex);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void createTextureSampler();
	void createDepthResources();
	void createColorResources();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);
	void compileShaders();
	void initPersistentArtResource();
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	VkSampleCountFlagBits getMaxUsableSampleCount();
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
		VkDebugUtilsMessageTypeFlagsEXT messageType, 
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
		void* pUserData) {

		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

};

