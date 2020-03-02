#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#define GLFW_INCLUDE_VULKAN
#define SHOW_FPS
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <set>
#include <algorithm>
#include "shader_mgr.h"
#include "vulkan_utility.h"
#include "camera.h"
#include "texture_mgr.h"
#include "directional_light.h"
#include "mesh_mgr.h"
#include "math.h"
#include "input_mgr.h"
#include "VulkanBuffer.hpp"
#include "VulkanglTFModel.hpp"
#include "render_element.h"
#include "view_frustum_culler.h"
//right hand axis
//using namespace std;
/*
Scalars have to be aligned by N(= 4 bytes given 32 bit floats).
A vec2 must be aligned by 2N(= 8 bytes)
A vec3 or vec4 must be aligned by 4N(= 16 bytes)
A nested structure must be aligned by the base alignment of its members rounded up to a multiple of 16.
A mat4 matrix must have the same alignment as a vec4.
*/
struct UboGlobalParams {
	/*alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 model_for_normal;*/
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec3 light_direction;
	alignas(16) glm::vec3 light_color;
	alignas(16) glm::vec3 cam_world_pos;
	alignas(4) float max_reflection_lod;
	alignas(4) float game_time;
};

//struct UboDirectionalLights {
//	alignas(16) glm::vec3 direction;
//	alignas(16) glm::vec3 color;
//};
//
//struct UboParams {
//	alignas(16) glm::vec3 cam_world_pos;
//	alignas(4) float max_reflection_lod;
//	alignas(4) float game_time;
//};

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
	ViewFrustumCuller vfc_;
	DirectionalLight main_light_;
	const int WINDOW_WIDTH = 1920;
	const int WINDOW_HEIGHT = 1080;
	bool vsync_ = false;
	float delta_time_ = 0.0f;
	vector<RenderElement> render_elements_;
	friend class RenderElement;
	friend class MSOCManager;
private:
	GLFWwindow *window_;
	InputManager input_mgr_;
	VkDebugUtilsMessengerEXT debug_messenger_;
	VulkanUtility::VulkanContex vk_contex_;

	vector<VkCommandBuffer> command_buffers_;//with the size of swapChain image count.

	//struct MVPBufferSturct {
	//	VkBuffer mvp_buffer;
	//	VkDeviceMemory mvp_buffer_memory;
	//};
	vector<VulkanBuffer> global_params_ubos_;


	//vector<array<glm::mat4, 2>> pushConstants_;
	//struct DrctLightBufferSturct {
	//	VkBuffer directional_light_buffer;
	//	VkDeviceMemory directional_light_buffer_memory;
	//};
	//vector<VulkanBuffer> directional_light_ubos_;

	////struct ParamsBufferSturct {
	////	VkBuffer params_buffer;
	////	VkDeviceMemory params_buffer_memory;
	////};
	//vector<VulkanBuffer> params_ubos_;

	vector<string> material_tex_names_ = { 
		"basecolor", 
		"normal", 
		"metallic", 
		"roughness", 
		"occlusion", 
		"env_irradiance_cubemap",
		"env_specular_cubemap",
		"env_brdf_lut" };

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
	/*Although many drivers and platforms trigger VK_ERROR_OUT_OF_DATE_KHR automatically after a window resize, 
	it is not guaranteed to happen. That's why we'll add some extra code to also handle resizes explicitly. */
	bool framebuffer_resized_ = false;
	bool is_render_skybox = true;
public:
	HakunaRenderer() :
		main_light_(vec3(1,1,1),vec3(1.0,0.9,0.8),1.5f){
		InitWindow();
		input_mgr_.Init(window_);
		InitVulkan();
		MSOCManager::GetInstance()->InitMSOCManager(this);
		Camera temp_cam(60.f, vk_contex_.vulkan_swapchain.extent_.width / (float)vk_contex_.vulkan_swapchain.extent_.height, 700.f, 0.01f, vec3(0, 0.2, 1), vec3(0, 0, 0));
		cam_ = temp_cam;
		cam_.SetupCameraContrl(input_mgr_);
		vfc_.SetOwnerCamera(&cam_);
	}
	~HakunaRenderer() {
		Cleanup();
	}
	void Loop()
	{
		auto start_time_for_cal_fps = std::chrono::high_resolution_clock::now();
		auto start_time_for_cal_delta = std::chrono::high_resolution_clock::now();
#ifdef SHOW_FPS
		int frame_cnt = 0;
#endif
		while (!glfwWindowShouldClose(window_)) {
			glfwPollEvents();
			cam_.UpdatePerFrame(delta_time_);
			DrawFrame();
			auto end_time = std::chrono::high_resolution_clock::now();
			delta_time_ = std::chrono::duration<float, std::chrono::seconds::period>(end_time - start_time_for_cal_delta).count();
			start_time_for_cal_delta = std::chrono::high_resolution_clock::now();
#ifdef SHOW_FPS			
			frame_cnt++;
			if (frame_cnt == 500)
			{
				float tmp_fps = 500.f / std::chrono::duration<float, std::chrono::seconds::period>(end_time - start_time_for_cal_fps).count();
				glfwSetWindowTitle(window_, ("Hakuna[FPS:" + std::to_string(tmp_fps) + "]").c_str());
				frame_cnt = 0;
				start_time_for_cal_fps = std::chrono::high_resolution_clock::now();
			}	
#endif
		}
		vkDeviceWaitIdle(vk_contex_.vulkan_device.logical_device);
	}

	void GetWindowSize(int* width,int* height);
private:
	void InitVulkan();//stay
	void SetupDebugMessenger();//stay
	void InitWindow();//stay
	void Cleanup();
	void ReCreateSwapChain();//13
	void CreateCommandBuffers();
	void RecordCommandBuffers(uint32_t currentImage);

	void CullOccludedRenderElement(uint32_t currentImage);
	void DrawFrame();
	void CreateSyncObjects();
	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t currentImage);
	void CreateGraphicPipeline();
	void CreateOpaqueDescriptorSets();
	void CreateSkyboxDescriptorSets();
	enum class EnvCubemapType {
		ECT_SPECULAR,
		ECT_DIFFUSE,
	};
	std::shared_ptr<Texture> GeneratePrefilterEnvCubemap(EnvCubemapType env_cubemap_type);
	std::shared_ptr<Texture> GenerateBRDFLUT();

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

