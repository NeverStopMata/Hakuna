#include "HakunaRenderer.h"
//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>
void HakunaRenderer::InitVulkan()
{
	VulkanUtility::CreateInstance(vk_contex_);
	SetupDebugMessenger();
	/*The window surface needs to be created right after the instance creation, because
	it can actually influence the physical device selection.*/
	VulkanUtility::CreateSurface(vk_contex_, window_);
	VulkanUtility::PickPhysicalDevice(vk_contex_);
	VulkanUtility::CreateLogicalDevice(vk_contex_);

	VulkanUtility::CreateSwapChain(vk_contex_,window_, vsync_);
	VulkanUtility::CreateImageViewsForSwapChain(vk_contex_);
	VulkanUtility::CreateRenderPass(vk_contex_);
	VulkanUtility::CreateDescriptorSetLayout(vk_contex_);
	CreateGraphicPipeline();
	VulkanUtility::CreateCommandPools(vk_contex_);

	VulkanUtility::CreateColorResources(vk_contex_);
	VulkanUtility::CreateDepthResources(vk_contex_);
	VulkanUtility::CreateFramebuffers(vk_contex_);
	VulkanUtility::CreateDescriptorPool(vk_contex_);
	CreateUniformBuffers();

	texture_mgr_.CreateTextureCube(this->vk_contex_, VK_FORMAT_R16G16B16A16_SFLOAT, "./textures/skybox_tex/hdr/uffizi_cube.ktx", "sky_texcube");
	texture_mgr_.CreateTexture2D(this->vk_contex_, VK_FORMAT_R8G8B8A8_UNORM,"textures/skybox_tex/sky.png", "sky_tex2d");
	texture_mgr_.CreateTexture2D(this->vk_contex_, VK_FORMAT_R8G8B8A8_UNORM, "textures/skybox_tex/diffuse_convolution.png", "diffuse_convolution");
	texture_mgr_.CreateTexture2D(this->vk_contex_, VK_FORMAT_R8G8B8A8_UNORM, "textures/gun_basecolor.png", "basecolor");
	texture_mgr_.CreateTexture2D(this->vk_contex_, VK_FORMAT_R8G8B8A8_UNORM, "textures/gun_metallic.png", "metallic");
	texture_mgr_.CreateTexture2D(this->vk_contex_, VK_FORMAT_R8G8B8A8_UNORM, "textures/gun_normal.png", "normal");
	texture_mgr_.CreateTexture2D(this->vk_contex_, VK_FORMAT_R8G8B8A8_UNORM, "textures/gun_occlusion.png", "occlusion");
	texture_mgr_.CreateTexture2D(this->vk_contex_, VK_FORMAT_R8G8B8A8_UNORM, "textures/gun_roughness.png", "roughness");
	mesh_mgr_.Init(&vk_contex_);
	mesh_mgr_.LoadModelFromFile("models/gun.obj", "gun");
	mesh_mgr_.LoadModelFromFile("models/sky.obj", "sky");
	CreateOpaqueDescriptorSets();
	CreateSkyboxDescriptorSets();
	CreateCommandBuffers();
	CreateSyncObjects();

}

void HakunaRenderer::InitWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window_ = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hakuna", nullptr, nullptr);
	glfwSetWindowUserPointer(window_, this);
	glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
}

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

void HakunaRenderer::CreateGraphicPipeline() {
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
	auto bindingDescription = Vertex::GetBindingDescription();
	auto attributeDescriptions = Vertex::GetAttributeDescriptions();
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
	viewport.width = (float)vk_contex_.swapchain_extent.width;
	viewport.height = (float)vk_contex_.swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	//用于裁剪
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = vk_contex_.swapchain_extent;

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
	multisampling.minSampleShading = .9f; // min fraction for sample shading; closer to one is smoother
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
	pipelineLayoutInfo.pSetLayouts = &vk_contex_.descriptor_set_layout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(vk_contex_.logical_device, &pipelineLayoutInfo, nullptr, &vk_contex_.pipeline_layout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	/*referencing the array of VkPipelineShaderStageCreateInfo structs.*/
	pipelineInfo.stageCount = shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
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

	pipelineInfo.layout = vk_contex_.pipeline_layout;

	pipelineInfo.renderPass = vk_contex_.renderpass;
	pipelineInfo.subpass = 0;

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	shaderStages[0] = shader_mgr_.LoadShader(vk_contex_, "shaders/compiled_shaders/opaque_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = shader_mgr_.LoadShader(vk_contex_, "shaders/compiled_shaders/opaque_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	if (vkCreateGraphicsPipelines(vk_contex_.logical_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vk_contex_.pipeline_struct.opaque_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	shaderStages[0] = shader_mgr_.LoadShader(vk_contex_, "shaders/compiled_shaders/skybox_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = shader_mgr_.LoadShader(vk_contex_, "shaders/compiled_shaders/skybox_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	if (vkCreateGraphicsPipelines(vk_contex_.logical_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vk_contex_.pipeline_struct.skybox_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
	
}


void HakunaRenderer::CreateCommandBuffers() {
	command_buffers_.resize(vk_contex_.swapchain_framebuffers.size());
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = vk_contex_.graphic_command_pool;
	//We won't make use of the secondary command buffer functionality here,
	//but you can imagine that it's helpful to reuse common operations from primary command buffers.
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)command_buffers_.size();
	if (vkAllocateCommandBuffers(vk_contex_.logical_device, &allocInfo, command_buffers_.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < command_buffers_.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		//The command buffer can be resubmitted while it is also already pending execution.
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional
		if (vkBeginCommandBuffer(command_buffers_[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = vk_contex_.renderpass;
		renderPassInfo.framebuffer = vk_contex_.swapchain_framebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = vk_contex_.swapchain_extent;
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		/*The range of depths in the depth buffer is 0.0 to 1.0 in Vulkan, where 1.0 lies at the far view plane and 0.0 at the near view plane. */
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();
		vkCmdBeginRenderPass(command_buffers_[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			VkBuffer vertexBuffers[] = { mesh_mgr_.GetMeshByName("gun")->vertex_buffer_ };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindDescriptorSets(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_contex_.pipeline_layout, 0, 1, &opaque_descriptor_sets_[i], 0, nullptr);
			vkCmdBindVertexBuffers(command_buffers_[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(command_buffers_[i], mesh_mgr_.GetMeshByName("gun")->index_buffer_, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_contex_.pipeline_struct.opaque_pipeline);
			vkCmdDrawIndexed(command_buffers_[i], static_cast<uint32_t>(mesh_mgr_.GetMeshByName("gun")->indices_.size()), 1, 0, 0, 0);
		}
		if(is_render_skybox)
		{
			VkBuffer vertexBuffers[] = { mesh_mgr_.GetMeshByName("sky")->vertex_buffer_ };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindDescriptorSets(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_contex_.pipeline_layout, 0, 1, &skybox_descriptor_sets_[i], 0, nullptr);
			vkCmdBindVertexBuffers(command_buffers_[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(command_buffers_[i], mesh_mgr_.GetMeshByName("sky")->index_buffer_, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_contex_.pipeline_struct.skybox_pipeline);
			vkCmdDrawIndexed(command_buffers_[i], static_cast<uint32_t>(mesh_mgr_.GetMeshByName("sky")->indices_.size()), 1, 0, 0, 0);
		}
		
		vkCmdEndRenderPass(command_buffers_[i]);
		if (vkEndCommandBuffer(command_buffers_[i]) != VK_SUCCESS) {
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
void HakunaRenderer::DrawFrame(){
	//only after the execution of the cmd buffer with the same frame index has been finished(coresponding fence beging signaled),
	//the function will go on.
	vkWaitForFences(vk_contex_.logical_device, 1, &in_flight_fences_[current_frame_], VK_TRUE, std::numeric_limits<uint64_t>::max());
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(vk_contex_.logical_device, vk_contex_.swapchain, std::numeric_limits<uint64_t>::max(), image_available_semaphores_[current_frame_], VK_NULL_HANDLE, &imageIndex);

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
	VkSemaphore image_acquired_semaphores[] = { image_available_semaphores_[current_frame_] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };//需要等待的阶段 颜色输出到attachment才需要等待imageAvailable,vert shader可以先开始
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = image_acquired_semaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &command_buffers_[imageIndex];
	////The signalSemaphoreCount and pSignalSemaphores parameters specify which semaphores to signal once the command buffer(s) have finished execution.
	VkSemaphore render_finish_semaphores[] = { render_finished_semaphores_[current_frame_] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = render_finish_semaphores;

	// fence wont be reset automatically while semaphores can be reset as soon as it is caught.
	vkResetFences(vk_contex_.logical_device, 1, &in_flight_fences_[current_frame_]);

	//after finishing the execution of the submitted cmd, inFlightFences[i] will be signaled.
	if (vkQueueSubmit(vk_contex_.graphics_queue, 1, &submitInfo, in_flight_fences_[current_frame_]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = render_finish_semaphores;
	VkSwapchainKHR swapChains[] = { vk_contex_.swapchain };
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

void HakunaRenderer::ReCreateSwapChain() {
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window_, &width, &height);
		glfwWaitEvents();
	}
	cam_.UpdateAspect((float)width / (float)height);
	vkDeviceWaitIdle(vk_contex_.logical_device);
	VulkanUtility::CleanupSwapChain(vk_contex_, command_buffers_);
	VulkanUtility::CreateSwapChain(vk_contex_, window_,vsync_);
	VulkanUtility::CreateImageViewsForSwapChain(vk_contex_);
	VulkanUtility::CreateRenderPass(vk_contex_);
	CreateGraphicPipeline();
	VulkanUtility::CreateColorResources(vk_contex_);
	VulkanUtility::CreateDepthResources(vk_contex_);
	VulkanUtility::CreateFramebuffers(vk_contex_);
	CreateCommandBuffers();
	
}


void HakunaRenderer::CreateUniformBuffers() {
	std::vector<uint32_t> uboQueueFamilyIndices{
		static_cast<uint32_t>(vk_contex_.queue_family_indices.transferFamily),
		static_cast<uint32_t>(vk_contex_.queue_family_indices.graphicsFamily) };

	mvp_ubos_.resize(vk_contex_.swapchain_images.size());
	directional_light_ubos_.resize(vk_contex_.swapchain_images.size());
	params_ubos_.resize(vk_contex_.swapchain_images.size());

	for (size_t i = 0; i < vk_contex_.swapchain_images.size(); i++) {
		VulkanUtility::CreateBuffer(vk_contex_,
			sizeof(UboMVP),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			mvp_ubos_[i].mvp_buffer,
			mvp_ubos_[i].mvp_buffer_memory,
			1, nullptr);

		VulkanUtility::CreateBuffer(vk_contex_,
			sizeof(UboDirectionalLights),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			directional_light_ubos_[i].directional_light_buffer,
			directional_light_ubos_[i].directional_light_buffer_memory,
			1, nullptr);

		VulkanUtility::CreateBuffer(vk_contex_,
			sizeof(UboParams),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			params_ubos_[i].params_buffer,
			params_ubos_[i].params_buffer_memory,
			1, nullptr);
	}
}

void HakunaRenderer::UpdateUniformBuffer(uint32_t currentImage) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	UboMVP ubo_mvps = {};
	ubo_mvps.model = glm::rotate(
		glm::mat4(1.0f), 
		0.1f * time * glm::radians(90.0f), 
		glm::vec3(0.0f, 1.0f, 0.0f));
	ubo_mvps.model_for_normal = glm::inverse(glm::transpose(ubo_mvps.model));
	ubo_mvps.view = cam_.GetViewMatrix();
	ubo_mvps.proj = cam_.GetProjMatrix();
	//GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. 
	ubo_mvps.proj[1][1] *= -1;
	void* data;
	vkMapMemory(vk_contex_.logical_device, mvp_ubos_[currentImage].mvp_buffer_memory, 0, sizeof(ubo_mvps), 0, &data);
	memcpy(data, &ubo_mvps, sizeof(ubo_mvps));
	vkUnmapMemory(vk_contex_.logical_device, mvp_ubos_[currentImage].mvp_buffer_memory);


	UboDirectionalLights ubo_lights = {};
	ubo_lights.color = main_light_.GetScaledColor();
	ubo_lights.direction = main_light_.GetDirection();
	vkMapMemory(vk_contex_.logical_device, directional_light_ubos_[currentImage].directional_light_buffer_memory, 0, sizeof(ubo_lights), 0, &data);
	memcpy(data, &ubo_lights, sizeof(ubo_lights));
	vkUnmapMemory(vk_contex_.logical_device, directional_light_ubos_[currentImage].directional_light_buffer_memory);

	UboParams ubo_params = {};
	ubo_params.cam_world_pos = cam_.GetWorldPos();
	vkMapMemory(vk_contex_.logical_device, params_ubos_[currentImage].params_buffer_memory, 0, sizeof(ubo_params), 0, &data);
	memcpy(data, &ubo_params, sizeof(ubo_params));
	vkUnmapMemory(vk_contex_.logical_device, params_ubos_[currentImage].params_buffer_memory);
}
void HakunaRenderer::Cleanup()
{
	shader_mgr_.CleanShaderModules(vk_contex_);
	VulkanUtility::CleanupSwapChain(vk_contex_, command_buffers_);
	vkDestroyDescriptorSetLayout(vk_contex_.logical_device, vk_contex_.descriptor_set_layout, nullptr);
	for (size_t i = 0; i < vk_contex_.swapchain_images.size(); i++) {
		vkDestroyBuffer(vk_contex_.logical_device, mvp_ubos_[i].mvp_buffer, nullptr);
		vkFreeMemory(vk_contex_.logical_device, mvp_ubos_[i].mvp_buffer_memory, nullptr);
		vkDestroyBuffer(vk_contex_.logical_device, directional_light_ubos_[i].directional_light_buffer, nullptr);
		vkFreeMemory(vk_contex_.logical_device, directional_light_ubos_[i].directional_light_buffer_memory, nullptr);
		vkDestroyBuffer(vk_contex_.logical_device, params_ubos_[i].params_buffer, nullptr);
		vkFreeMemory(vk_contex_.logical_device, params_ubos_[i].params_buffer_memory, nullptr);
	}
	mesh_mgr_.CleanUpMeshDict();
	texture_mgr_.CleanUpTextures(vk_contex_);
	vkDestroyDescriptorPool(vk_contex_.logical_device, vk_contex_.descriptor_pool, nullptr);
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

void HakunaRenderer::CreateOpaqueDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(vk_contex_.swapchain_images.size(), vk_contex_.descriptor_set_layout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = vk_contex_.descriptor_pool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(vk_contex_.swapchain_images.size());
	allocInfo.pSetLayouts = layouts.data();
	opaque_descriptor_sets_.resize(vk_contex_.swapchain_images.size());
	if (vkAllocateDescriptorSets(vk_contex_.logical_device, &allocInfo, opaque_descriptor_sets_.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
	for (size_t i = 0; i < vk_contex_.swapchain_images.size(); i++) {
		VkDescriptorBufferInfo mvp_buffer_Info = {};
		mvp_buffer_Info.buffer = mvp_ubos_[i].mvp_buffer;
		mvp_buffer_Info.offset = 0;
		mvp_buffer_Info.range = sizeof(UboMVP);

		VkDescriptorBufferInfo direc_light_buffer_info = {};
		direc_light_buffer_info.buffer = directional_light_ubos_[i].directional_light_buffer;
		direc_light_buffer_info.offset = 0;
		direc_light_buffer_info.range = sizeof(UboDirectionalLights);

		VkDescriptorBufferInfo params_buffer_info = {};
		params_buffer_info.buffer = params_ubos_[i].params_buffer;
		params_buffer_info.offset = 0;
		params_buffer_info.range = sizeof(UboParams);

		std::vector<VkWriteDescriptorSet> descriptorWrites(3 + material_tex_names_.size());

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = opaque_descriptor_sets_[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &mvp_buffer_Info;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = opaque_descriptor_sets_[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &direc_light_buffer_info;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = opaque_descriptor_sets_[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &params_buffer_info;

		vector<VkDescriptorImageInfo> descriptor_image_infos;
		descriptor_image_infos.resize(material_tex_names_.size());
		for (int j = 0; j < descriptor_image_infos.size(); j++) {
			descriptor_image_infos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			descriptor_image_infos[j].imageView = texture_mgr_.GetTextureByName(material_tex_names_[j])->texture_image_view;
			descriptor_image_infos[j].sampler = texture_mgr_.GetTextureByName(material_tex_names_[j])->texture_sampler;
		}

		for (int k = 3; k < descriptorWrites.size(); k++) {
			descriptorWrites[k].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[k].dstSet = opaque_descriptor_sets_[i];
			descriptorWrites[k].dstBinding = k;
			descriptorWrites[k].dstArrayElement = 0;
			descriptorWrites[k].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[k].descriptorCount = 1;
			descriptorWrites[k].pImageInfo = &descriptor_image_infos[k - 3];
		}
		vkUpdateDescriptorSets(vk_contex_.logical_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void HakunaRenderer::CreateSkyboxDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(vk_contex_.swapchain_images.size(), vk_contex_.descriptor_set_layout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = vk_contex_.descriptor_pool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(vk_contex_.swapchain_images.size());
	allocInfo.pSetLayouts = layouts.data();
	skybox_descriptor_sets_.resize(vk_contex_.swapchain_images.size());
	if (vkAllocateDescriptorSets(vk_contex_.logical_device, &allocInfo, skybox_descriptor_sets_.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
	int i = 0;
	i++;
	i = i * 5;

	for (size_t i = 0; i < vk_contex_.swapchain_images.size(); i++) {
		VkDescriptorBufferInfo mvp_buffer_Info = {};
		mvp_buffer_Info.buffer = mvp_ubos_[i].mvp_buffer;
		mvp_buffer_Info.offset = 0;
		mvp_buffer_Info.range = sizeof(UboMVP);

		VkDescriptorBufferInfo direc_light_buffer_info = {};
		direc_light_buffer_info.buffer = directional_light_ubos_[i].directional_light_buffer;
		direc_light_buffer_info.offset = 0;
		direc_light_buffer_info.range = sizeof(UboDirectionalLights);

		VkDescriptorBufferInfo params_buffer_info = {};
		params_buffer_info.buffer = params_ubos_[i].params_buffer;
		params_buffer_info.offset = 0;
		params_buffer_info.range = sizeof(UboParams);

		VkDescriptorImageInfo descriptor_image_info;
		descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		if (1) {
			descriptor_image_info.imageView = texture_mgr_.GetTextureByName("sky_texcube")->texture_image_view;
			descriptor_image_info.sampler = texture_mgr_.GetTextureByName("sky_texcube")->texture_sampler;
		}
		else {
			descriptor_image_info.imageView = texture_mgr_.GetTextureByName("sky_tex2d")->texture_image_view;
			descriptor_image_info.sampler = texture_mgr_.GetTextureByName("sky_tex2d")->texture_sampler;
		}
		
		

		std::vector<VkWriteDescriptorSet> descriptorWrites(4);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = skybox_descriptor_sets_[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &mvp_buffer_Info;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = skybox_descriptor_sets_[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &direc_light_buffer_info;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = skybox_descriptor_sets_[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &params_buffer_info;

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = skybox_descriptor_sets_[i];
		descriptorWrites[3].dstBinding = 3;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pImageInfo = &descriptor_image_info;
		vkUpdateDescriptorSets(vk_contex_.logical_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
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

