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
#include "vulkan_core.h"
#include "camera.h"
#include "texture_mgr.h"
using namespace std;
const int MAX_FRAMES_IN_FLIGHT = 2;
const string TEXTURE_PATH = "textures/chalet.jpg";

//const vector<const char*> deviceExtensions = {
//	VK_KHR_SWAPCHAIN_EXTENSION_NAME//显式地设置swap chain 的扩展（虽然只要presentation的queue family存在就可以说明该物理设备支持输出到屏幕）
//};

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





VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance, 
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(
	VkInstance instance, 
	VkDebugUtilsMessengerEXT debugMessenger, 
	const VkAllocationCallbacks* pAllocator);
class HakunaRenderer
{
public:
	shared_ptr<Camera> cam_;
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;
private:
	TextureMgr texture_mgr_;
	GLFWwindow *window_;
	VkDebugUtilsMessengerEXT debug_messenger_;
	VulkanCore::VulkanContex vk_contex_;
	
	//VkSurfaceKHR surface_;
	VkSwapchainKHR swapchain_;
	vector<VkImage> swapchain_images_;
	VkFormat swapchain_image_format_;
	VkExtent2D swapchain_extent_;
	vector<VkImageView> swapchain_image_views_;
	VkRenderPass renderpass_;
	VkDescriptorSetLayout descriptor_set_layout_;
	VkPipelineLayout pipeline_layout_;
	VkPipeline graphics_pipeline_;
	vector<VkFramebuffer> swapchain_framebuffers_;//with the size of swapChain image count.


	vector<VkCommandBuffer> commandBuffers;//with the size of swapChain image count.
	VkImage depth_image_;
	VkDeviceMemory depth_image_memory_;
	VkImageView depth_image_view_;

	//used for multi-sampling
	VkImage color_image_;
	VkDeviceMemory color_image_memory_;
	VkImageView color_image_view_;

	//VkImage texture_image_;
	//VkDeviceMemory texture_image_memory_;
	//VkImageView texture_image_view_;
	TextureMgr::Texture diffuse_texture_;

	VkBuffer vertex_buffer_;
	VkDeviceMemory vertex_buffer_memory_;
	VkBuffer index_buffer_;
	VkDeviceMemory index_buffer_memory_;
	vector<VkBuffer> uniform_buffers_;
	vector<VkDeviceMemory> uniform_buffers_memory_;



	VkDescriptorPool descriptor_pool_;
	vector<VkDescriptorSet> descriptor_sets_;

	vector<VkSemaphore> image_available_semaphores_;
	vector<VkSemaphore> render_finished_semaphores_;
	vector<VkFence> in_flight_fences_;
	size_t current_frame_ = 0;
	unique_ptr<VertexDataManager> vertex_data_mgr_;
	/*Although many drivers and platforms trigger VK_ERROR_OUT_OF_DATE_KHR automatically after a window resize, 
	it is not guaranteed to happen. That's why we'll add some extra code to also handle resizes explicitly. */
	bool framebuffer_resized_ = false;
public:
	HakunaRenderer() {
		LoadPersistentArtResource();
		InitWindow();
		InitVulkan(window_);
		cam_ = make_shared<Camera>(45.f, swapchain_extent_.width / (float)swapchain_extent_.height, 100.f, 0.01f, vec3(2, 2, 2), vec3(0, 0, 0));
	}
	~HakunaRenderer() {
		Cleanup();
	}
	void Loop()
	{
		while (!glfwWindowShouldClose(window_)) {
			glfwPollEvents();
			drawFrame();
		}
		vkDeviceWaitIdle(vk_contex_.logical_device);
	}

private:
	void InitVulkan(GLFWwindow* window);//stay
	//void CreateInstance();//1
	//void CreateSurface();//stay
	//void PickPhysicalDevice();//2
	void SetupDebugMessenger();//3
	void InitWindow();//stay
	void Cleanup();
	void cleanupSwapChain();//4
	//bool checkValidationLayerSupport();//5
	//vector<const char*> getRequiredExtensions();//6
	//bool isDeviceSuitable(VkPhysicalDevice device);//7
	bool checkDeviceExtensionsSupport(VkPhysicalDevice device);//8
	//VulkanCore::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);//9
	//SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);//10
	//void CreateLogicalDevice();//11
	void CreateSwapChain();//12
	void ReCreateSwapChain();//13
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void CreateImageViewsForSwapChain();
	void CreateGraphicsPipeline();
	VkShaderModule createShaderModule(const vector<char>& code); 
	void CreateRenderPass();
	void CreateFramebuffers();
	void CreateCommandPools();
	void CreateCommandBuffers();
	void drawFrame();
	void CreateSyncObjects();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void CreateDescriptorSetLayout();
	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t currentImage);
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	VkCommandBuffer beginSingleTimeCommands(uint32_t queueFamilyIndex);
	void endSingleTimeCommands(VkCommandBuffer commandBuffer, uint32_t queueFamilyIndex);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void CreateDepthResources();
	void CreateColorResources();
	VkFormat findSupportedFormat(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	//bool hasStencilComponent(VkFormat format);
	void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	VkSampleCountFlagBits getMaxUsableSampleCount();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
		VkDebugUtilsMessageTypeFlagsEXT messageType, 
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
		void* pUserData) {

		cerr << "validation layer: " << pCallbackData->pMessage << endl;

		return VK_FALSE;
	}


	void LoadPersistentArtResource();

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<HakunaRenderer*>(glfwGetWindowUserPointer(window));
		app->framebuffer_resized_ = true;
	}

};

