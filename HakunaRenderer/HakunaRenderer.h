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
#include "shader_manager.h"
#include "vulkan_utility.h"
#include "camera.h"
#include "texture_mgr.h"
#include "directional_light.h"
#include "mesh_mgr.h"
using namespace std;
const int MAX_FRAMES_IN_FLIGHT = 2;


/*
Scalars have to be aligned by N(= 4 bytes given 32 bit floats).
A vec2 must be aligned by 2N(= 8 bytes)
A vec3 or vec4 must be aligned by 4N(= 16 bytes)
A nested structure must be aligned by the base alignment of its members rounded up to a multiple of 16.
A mat4 matrix must have the same alignment as a vec4.
*/
struct UboMVP {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 model_for_normal;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct UboDirectionalLights {
	alignas(16) glm::vec3 direction;
	alignas(16) glm::vec3 color;
};

struct UboParams {
	alignas(16) glm::vec3 cam_world_pos;
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
	Camera cam_;
	DirectionalLight main_light_;
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;
	bool vsync_ = false;
private:
	TextureMgr texture_mgr_;
	GLFWwindow *window_;
	VkDebugUtilsMessengerEXT debug_messenger_;
	VulkanUtility::VulkanContex vk_contex_;

	vector<VkCommandBuffer> command_buffers_;//with the size of swapChain image count.

	struct MVPBufferSturct {
		VkBuffer mvp_buffer;
		VkDeviceMemory mvp_buffer_memory;
	};
	vector<MVPBufferSturct> mvp_ubos_;

	struct DrctLightBufferSturct {
		VkBuffer directional_light_buffer;
		VkDeviceMemory directional_light_buffer_memory;
	};
	vector<DrctLightBufferSturct> directional_light_ubos_;

	struct ParamsBufferSturct {
		VkBuffer params_buffer;
		VkDeviceMemory params_buffer_memory;
	};
	vector<ParamsBufferSturct> params_ubos_;



	vector<string> material_tex_names_ = { "basecolor", "normal", "metallic", "roughness", "occlusion", "diffuse_convolution"};
	vector<VkDescriptorSet> opaque_descriptor_sets_;
	vector<VkDescriptorSet> skybox_descriptor_sets_;
	struct {
		VkDescriptorSet opaque_object;
		VkDescriptorSet skybox;
	}descriptor_sets_;
	
	vector<VkSemaphore> image_available_semaphores_;
	vector<VkSemaphore> render_finished_semaphores_;
	vector<VkFence> in_flight_fences_;
	size_t current_frame_ = 0;
	MeshMgr mesh_mgr_;
	ShaderManager shader_mgr_;
	/*Although many drivers and platforms trigger VK_ERROR_OUT_OF_DATE_KHR automatically after a window resize, 
	it is not guaranteed to happen. That's why we'll add some extra code to also handle resizes explicitly. */
	bool framebuffer_resized_ = false;
	bool is_render_skybox = true;
public:
	HakunaRenderer():
		main_light_(vec3(1,1,1),vec3(1.0,0.9,0.8),1.5f){

		InitWindow();
		InitVulkan();
		Camera temp_cam(90.f, vk_contex_.swapchain_extent.width / (float)vk_contex_.swapchain_extent.height, 100.f, 0.01f, vec3(0, 0.2, 1), vec3(0, 0, 0));
		cam_ = temp_cam;
	}
	~HakunaRenderer() {
		Cleanup();
	}
	void Loop()
	{
#ifdef NDEBUG
#else
		float min_frame_rate = 10000;
#endif
		while (!glfwWindowShouldClose(window_)) {
			glfwPollEvents();
			auto start_time = std::chrono::high_resolution_clock::now();
			DrawFrame();

#ifdef NDEBUG
#else
			auto end_time = std::chrono::high_resolution_clock::now();
			float tmp_fps = 1.f / std::chrono::duration<float, std::chrono::seconds::period>(end_time - start_time).count();
			if (tmp_fps < min_frame_rate) {
				min_frame_rate = tmp_fps;
				std::cout << "min fps" << min_frame_rate << std::endl;
			}
			std::cout << "current fps" << tmp_fps << std::endl;
#endif
		}
		vkDeviceWaitIdle(vk_contex_.logical_device);
	}

private:
	void InitVulkan();//stay
	void SetupDebugMessenger();//stay
	void InitWindow();//stay
	void Cleanup();
	void ReCreateSwapChain();//13
	void CreateCommandBuffers();
	void DrawFrame();
	void CreateSyncObjects();
	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t currentImage);
	void CreateGraphicPipeline();
	void CreateOpaqueDescriptorSets();
	void CreateSkyboxDescriptorSets();
	//void CreateIrradianceMap();



	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
		VkDebugUtilsMessageTypeFlagsEXT messageType, 
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
		void* pUserData) {

		cerr << "validation layer: " << pCallbackData->pMessage << endl;

		return VK_FALSE;
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<HakunaRenderer*>(glfwGetWindowUserPointer(window));
		app->framebuffer_resized_ = true;
	}
};

