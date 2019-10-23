#include "HakunaRenderer.h"
void HakunaRenderer::InitVulkan()
{
	VulkanUtility::CreateInstance(vk_contex_);
	SetupDebugMessenger();
	/*The window surface needs to be created right after the instance creation, because
	it can actually influence the physical device selection.*/
	vk_contex_.vulkan_swapchain.CreateSurface(window_, vk_contex_.vk_instance);
	vk_contex_.vulkan_device.PickPhysicalDevice(vk_contex_.vk_instance, vk_contex_.vulkan_swapchain.surface_);
	vk_contex_.vulkan_device.CreateLogicalDevice();
	vk_contex_.vulkan_swapchain.Connect(vk_contex_.vk_instance, vk_contex_.vulkan_device.physical_device, vk_contex_.vulkan_device.logical_device);
	vk_contex_.vulkan_swapchain.CreateSwapchain(window_, vk_contex_.vulkan_device, false);
	VulkanUtility::CreateRenderPass(vk_contex_);
	VulkanUtility::CreateDescriptorSetLayout(vk_contex_);
	vk_contex_.vulkan_device.CreatePipelineCache();
	CreateGraphicPipeline();
	vk_contex_.vulkan_device.CreateCommandPools();
	VulkanUtility::CreateColorResources(vk_contex_);
	VulkanUtility::CreateDepthResources(vk_contex_);
	VulkanUtility::CreateFramebuffers(vk_contex_);
	VulkanUtility::CreateDescriptorPool(vk_contex_);
	CreateUniformBuffers();
	MeshMgr::GetInstance().Init(&vk_contex_);
	MeshMgr::GetInstance().LoadModelFromFile("resource/models/gun.obj", "gun");// temp debug
	//MeshMgr::GetInstance().CreateCubeMesh("gun", glm::vec3(5, 0.1, 5));
	MeshMgr::GetInstance().LoadModelFromFile("resource/models/sky.obj", "sky",glm::vec3(10,10,10));
	TextureMgr::GetInstance().AddTexture("sky_texcube", (new Texture(&vk_contex_))->LoadTextureCube("resource/textures/skybox_tex/hdr/gcanyon_cube.ktx"));
	TextureMgr::GetInstance().AddTexture("basecolor", (new Texture(&vk_contex_))->LoadTexture2D("resource/textures/gun_basecolor.dds"));
	TextureMgr::GetInstance().AddTexture("metallic", (new Texture(&vk_contex_))->LoadTexture2D("resource/textures/gun_metallic.dds"));
	TextureMgr::GetInstance().AddTexture("normal", (new Texture(&vk_contex_))->LoadTexture2D("resource/textures/gun_normal.dds"));
	TextureMgr::GetInstance().AddTexture("occlusion", (new Texture(&vk_contex_))->LoadTexture2D("resource/textures/gun_occlusion.dds"));
	TextureMgr::GetInstance().AddTexture("roughness", (new Texture(&vk_contex_))->LoadTexture2D("resource/textures/gun_roughness.dds"));
	TextureMgr::GetInstance().AddTexture("env_irradiance_cubemap", GeneratePrefilterEnvCubemap(EnvCubemapType::ECT_DIFFUSE));
	TextureMgr::GetInstance().AddTexture("env_specular_cubemap", GeneratePrefilterEnvCubemap(EnvCubemapType::ECT_SPECULAR));
	TextureMgr::GetInstance().AddTexture("env_brdf_lut", GenerateBRDFLUT());
	scene_.loadFromFile("resource/gltf/Sponza/glTF/Sponza.gltf", &vk_contex_.vulkan_device, vk_contex_.vulkan_device.graphics_queue);
	CreateSyncObjects();
	CreateOpaqueDescriptorSets();
	CreateSkyboxDescriptorSets();
	CreateCommandBuffers();
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
	viewport.width = (float)vk_contex_.vulkan_swapchain.extent_.width;
	viewport.height = (float)vk_contex_.vulkan_swapchain.extent_.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	//用于裁剪
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = vk_contex_.vulkan_swapchain.extent_;

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
	multisampling.rasterizationSamples = vk_contex_.vulkan_device.msaasample_size;
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

	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.pSetLayouts = &vk_contex_.descriptor_set_layout;
	pipelineLayoutCI.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(vk_contex_.vulkan_device.logical_device, &pipelineLayoutCI, nullptr, &vk_contex_.pipeline_layout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineCI = {};
	pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	/*referencing the array of VkPipelineShaderStageCreateInfo structs.*/
	pipelineCI.stageCount = shaderStages.size();
	pipelineCI.pStages = shaderStages.data();
	/*the fixed - function stage.*/
	pipelineCI.pVertexInputState = &vertexInputInfo;
	pipelineCI.pInputAssemblyState = &inputAssembly;
	pipelineCI.pViewportState = &viewportState;
	pipelineCI.pRasterizationState = &rasterizer;
	pipelineCI.pMultisampleState = &multisampling;
	pipelineCI.pDepthStencilState = nullptr; // Optional
	pipelineCI.pColorBlendState = &colorBlending;
	pipelineCI.pDepthStencilState = &depthStencil;
	pipelineCI.pDynamicState = nullptr; // Optional
	pipelineCI.layout = vk_contex_.pipeline_layout;
	pipelineCI.renderPass = vk_contex_.renderpass;
	pipelineCI.subpass = 0;

	pipelineCI.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineCI.basePipelineIndex = -1; // Optional

	shaderStages[0] = ShaderMgr::GetInstance().LoadShader(vk_contex_, "resource/shaders/compiled_shaders/opaque_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = ShaderMgr::GetInstance().LoadShader(vk_contex_, "resource/shaders/compiled_shaders/opaque_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	if (vkCreateGraphicsPipelines(vk_contex_.vulkan_device.logical_device, vk_contex_.vulkan_device.pipeline_cache, 1, &pipelineCI, nullptr, &vk_contex_.pipeline_struct.opaque_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
	shaderStages[0] = ShaderMgr::GetInstance().LoadShader(vk_contex_, "resource/shaders/compiled_shaders/skybox_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = ShaderMgr::GetInstance().LoadShader(vk_contex_, "resource/shaders/compiled_shaders/skybox_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	if (vkCreateGraphicsPipelines(vk_contex_.vulkan_device.logical_device, vk_contex_.vulkan_device.pipeline_cache, 1, &pipelineCI, nullptr, &vk_contex_.pipeline_struct.skybox_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
}


void HakunaRenderer::CreateCommandBuffers() {
	command_buffers_.resize(vk_contex_.swapchain_framebuffers.size());
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = vk_contex_.vulkan_device.graphic_command_pool;
	//We won't make use of the secondary command buffer functionality here,
	//but you can imagine that it's helpful to reuse common operations from primary command buffers.
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)command_buffers_.size();
	if (vkAllocateCommandBuffers(vk_contex_.vulkan_device.logical_device, &allocInfo, command_buffers_.data()) != VK_SUCCESS) {
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
		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = vk_contex_.renderpass;
		renderPassBeginInfo.framebuffer = vk_contex_.swapchain_framebuffers[i];
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = vk_contex_.vulkan_swapchain.extent_;
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		/*The range of depths in the depth buffer is 0.0 to 1.0 in Vulkan, where 1.0 lies at the far view plane and 0.0 at the near view plane. */
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();
		vkCmdBeginRenderPass(command_buffers_[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			VkBuffer vertexBuffers[] = { MeshMgr::GetInstance().GetMeshByName("gun")->vertex_buffer_.buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindDescriptorSets(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_contex_.pipeline_layout, 0, 1, &opaque_descriptor_sets_[i], 0, nullptr);
			vkCmdBindVertexBuffers(command_buffers_[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(command_buffers_[i], MeshMgr::GetInstance().GetMeshByName("gun")->index_buffer_.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_contex_.pipeline_struct.opaque_pipeline);
			vkCmdDrawIndexed(command_buffers_[i], static_cast<uint32_t>(MeshMgr::GetInstance().GetMeshByName("gun")->indices_.size()), 1, 0, 0, 0);
		}


		if(is_render_skybox)
		{
			VkBuffer vertexBuffers[] = { MeshMgr::GetInstance().GetMeshByName("sky")->vertex_buffer_.buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindDescriptorSets(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_contex_.pipeline_layout, 0, 1, &skybox_descriptor_sets_[i], 0, nullptr);
			vkCmdBindVertexBuffers(command_buffers_[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(command_buffers_[i], MeshMgr::GetInstance().GetMeshByName("sky")->index_buffer_.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_contex_.pipeline_struct.skybox_pipeline);
			vkCmdDrawIndexed(command_buffers_[i], static_cast<uint32_t>(MeshMgr::GetInstance().GetMeshByName("sky")->indices_.size()), 1, 0, 0, 0);
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
	vkWaitForFences(
		vk_contex_.vulkan_device.logical_device, 
		1, 
		&in_flight_fences_[current_frame_], 
		VK_TRUE, 
		std::numeric_limits<uint64_t>::max());
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(vk_contex_.vulkan_device.logical_device, vk_contex_.vulkan_swapchain.swapChain_, std::numeric_limits<uint64_t>::max(), image_available_semaphores_[current_frame_], VK_NULL_HANDLE, &imageIndex);

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
	vkResetFences(vk_contex_.vulkan_device.logical_device, 1, &in_flight_fences_[current_frame_]);

	//after finishing the execution of the submitted cmd, inFlightFences[i] will be signaled.
	if (vkQueueSubmit(vk_contex_.vulkan_device.graphics_queue, 1, &submitInfo, in_flight_fences_[current_frame_]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = render_finish_semaphores;
	VkSwapchainKHR swapChains[] = { vk_contex_.vulkan_swapchain.swapChain_ };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;//for check whether each image in the swap chain is presented successfully.
	result = vkQueuePresentKHR(vk_contex_.vulkan_device.present_queue, &presentInfo);
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
		if (vkCreateSemaphore(vk_contex_.vulkan_device.logical_device, &semaphoreInfo, nullptr,&image_available_semaphores_[i]) != VK_SUCCESS ||
			vkCreateSemaphore(vk_contex_.vulkan_device.logical_device, &semaphoreInfo, nullptr,&render_finished_semaphores_[i]) != VK_SUCCESS ||
			vkCreateFence(vk_contex_.vulkan_device.logical_device, &fenceInfo, nullptr,&in_flight_fences_[i]) != VK_SUCCESS){
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
	vkDeviceWaitIdle(vk_contex_.vulkan_device.logical_device);
	VulkanUtility::CleanupFramebufferAndPipeline(vk_contex_, command_buffers_);
	vk_contex_.vulkan_swapchain.CreateSwapchain(window_, vk_contex_.vulkan_device, false);
	VulkanUtility::CreateRenderPass(vk_contex_);
	CreateGraphicPipeline();
	VulkanUtility::CreateColorResources(vk_contex_);
	VulkanUtility::CreateDepthResources(vk_contex_);
	VulkanUtility::CreateFramebuffers(vk_contex_);
	CreateCommandBuffers();
	
}


void HakunaRenderer::CreateUniformBuffers() {
	std::vector<uint32_t> uboQueueFamilyIndices{
		static_cast<uint32_t>(vk_contex_.vulkan_device.queue_family_indices.transferFamily),
		static_cast<uint32_t>(vk_contex_.vulkan_device.queue_family_indices.graphicsFamily) };

	mvp_ubos_.resize(vk_contex_.vulkan_swapchain.imageCount_);
	directional_light_ubos_.resize(vk_contex_.vulkan_swapchain.imageCount_);
	params_ubos_.resize(vk_contex_.vulkan_swapchain.imageCount_);

	for (size_t i = 0; i < vk_contex_.vulkan_swapchain.imageCount_; i++) {

		vk_contex_.vulkan_device.CreateBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&mvp_ubos_[i],
			sizeof(UboMVP)
		);

		vk_contex_.vulkan_device.CreateBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&directional_light_ubos_[i],
			sizeof(UboDirectionalLights)
		);

		vk_contex_.vulkan_device.CreateBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&params_ubos_[i],
			sizeof(UboParams)
		);
	}
}

void HakunaRenderer::UpdateUniformBuffer(uint32_t currentImage) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	UboMVP ubo_mvps = {};
	ubo_mvps.model = glm::rotate(
		glm::mat4(1.0f), 
		0.2f * time * glm::radians(90.0f), 
		glm::vec3(0.0f, 1.0f, 0.0f));
	ubo_mvps.model_for_normal = glm::inverse(glm::transpose(ubo_mvps.model));
	ubo_mvps.view = cam_.GetViewMatrix();
	ubo_mvps.proj = cam_.GetProjMatrix();
	//GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. 
	ubo_mvps.proj[1][1] *= -1;
	mvp_ubos_[currentImage].map(sizeof(ubo_mvps));
	memcpy(mvp_ubos_[currentImage].mapped, &ubo_mvps, sizeof(ubo_mvps));
	mvp_ubos_[currentImage].unmap();


	UboDirectionalLights ubo_lights = {};
	ubo_lights.color = main_light_.GetScaledColor();
	ubo_lights.direction = main_light_.GetDirection();
	directional_light_ubos_[currentImage].map(sizeof(ubo_lights));
	memcpy(directional_light_ubos_[currentImage].mapped, &ubo_lights, sizeof(ubo_lights));
	directional_light_ubos_[currentImage].unmap();

	UboParams ubo_params = {};
	ubo_params.cam_world_pos = cam_.GetWorldPos();
	ubo_params.max_reflection_lod = static_cast<float>(TextureMgr::GetInstance().GetTextureByName("env_specular_cubemap")->miplevel_size_);
	params_ubos_[currentImage].map(sizeof(ubo_params));
	memcpy(params_ubos_[currentImage].mapped, &ubo_params, sizeof(ubo_params));
	params_ubos_[currentImage].unmap();
}
void HakunaRenderer::Cleanup()
{
	ShaderMgr::GetInstance().CleanShaderModules(vk_contex_);
	VulkanUtility::CleanupFramebufferAndPipeline(vk_contex_, command_buffers_);
	vk_contex_.vulkan_swapchain.Cleanup();
	vkDestroyDescriptorSetLayout(vk_contex_.vulkan_device.logical_device, vk_contex_.descriptor_set_layout, nullptr);
	for (size_t i = 0; i < vk_contex_.vulkan_swapchain.imageCount_; i++) {
		mvp_ubos_[i].destroy();
		directional_light_ubos_[i].destroy();
		params_ubos_[i].destroy();
	}
	MeshMgr::GetInstance().CleanUpMeshDict();
	TextureMgr::GetInstance().CleanUpTextures(vk_contex_);

	scene_.destroy(vk_contex_.vulkan_device.logical_device);//
	vkDestroyDescriptorPool(vk_contex_.vulkan_device.logical_device, vk_contex_.descriptor_pool, nullptr);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(vk_contex_.vulkan_device.logical_device, render_finished_semaphores_[i], nullptr);
		vkDestroySemaphore(vk_contex_.vulkan_device.logical_device, image_available_semaphores_[i], nullptr);
		vkDestroyFence(vk_contex_.vulkan_device.logical_device, in_flight_fences_[i], nullptr);
	}
	vk_contex_.vulkan_device.Cleanup();
	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(vk_contex_.vk_instance, debug_messenger_, nullptr);
	}
	vkDestroyInstance(vk_contex_.vk_instance, nullptr);

	cam_.ReleaseCameraContrl(input_mgr_);
	glfwDestroyWindow(window_);
	glfwTerminate();
}

void HakunaRenderer::CreateOpaqueDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(vk_contex_.vulkan_swapchain.imageCount_, vk_contex_.descriptor_set_layout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = vk_contex_.descriptor_pool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(vk_contex_.vulkan_swapchain.imageCount_);
	allocInfo.pSetLayouts = layouts.data();
	opaque_descriptor_sets_.resize(vk_contex_.vulkan_swapchain.imageCount_);
	if (vkAllocateDescriptorSets(vk_contex_.vulkan_device.logical_device, &allocInfo, opaque_descriptor_sets_.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
	for (size_t i = 0; i < vk_contex_.vulkan_swapchain.imageCount_; i++) {
		VkDescriptorBufferInfo mvp_buffer_Info = {};
		mvp_buffer_Info.buffer = mvp_ubos_[i].buffer;
		mvp_buffer_Info.offset = 0;
		mvp_buffer_Info.range = sizeof(UboMVP);

		VkDescriptorBufferInfo direc_light_buffer_info = {};
		direc_light_buffer_info.buffer = directional_light_ubos_[i].buffer;
		direc_light_buffer_info.offset = 0;
		direc_light_buffer_info.range = sizeof(UboDirectionalLights);

		VkDescriptorBufferInfo params_buffer_info = {};
		params_buffer_info.buffer = params_ubos_[i].buffer;
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

		for (int k = 3; k < descriptorWrites.size(); k++) {
			descriptorWrites[k].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[k].dstSet = opaque_descriptor_sets_[i];
			descriptorWrites[k].dstBinding = k;
			descriptorWrites[k].dstArrayElement = 0;
			descriptorWrites[k].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[k].descriptorCount = 1;
			descriptorWrites[k].pImageInfo = &TextureMgr::GetInstance().GetTextureByName(material_tex_names_[k-3])->descriptor_;
		}
		vkUpdateDescriptorSets(vk_contex_.vulkan_device.logical_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void HakunaRenderer::CreateSkyboxDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(vk_contex_.vulkan_swapchain.imageCount_, vk_contex_.descriptor_set_layout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = vk_contex_.descriptor_pool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(vk_contex_.vulkan_swapchain.imageCount_);
	allocInfo.pSetLayouts = layouts.data();
	skybox_descriptor_sets_.resize(vk_contex_.vulkan_swapchain.imageCount_);
	if (vkAllocateDescriptorSets(vk_contex_.vulkan_device.logical_device, &allocInfo, skybox_descriptor_sets_.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
	int i = 0;
	i++;
	i = i * 5;

	for (size_t i = 0; i < vk_contex_.vulkan_swapchain.imageCount_; i++) {
		VkDescriptorBufferInfo mvp_buffer_Info = {};
		mvp_buffer_Info.buffer = mvp_ubos_[i].buffer;
		mvp_buffer_Info.offset = 0;
		mvp_buffer_Info.range = sizeof(UboMVP);

		VkDescriptorBufferInfo direc_light_buffer_info = {};
		direc_light_buffer_info.buffer = directional_light_ubos_[i].buffer;
		direc_light_buffer_info.offset = 0;
		direc_light_buffer_info.range = sizeof(UboDirectionalLights);

		VkDescriptorBufferInfo params_buffer_info = {};
		params_buffer_info.buffer = params_ubos_[i].buffer;
		params_buffer_info.offset = 0;
		params_buffer_info.range = sizeof(UboParams);

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
		descriptorWrites[3].pImageInfo = &TextureMgr::GetInstance().GetTextureByName("sky_texcube")->descriptor_;
		vkUpdateDescriptorSets(vk_contex_.vulkan_device.logical_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

std::shared_ptr<Texture> HakunaRenderer::GeneratePrefilterEnvCubemap(EnvCubemapType env_cubemap_type)
{
	auto tStart = std::chrono::high_resolution_clock::now();
	const VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
	/* if the job takes a lot of time, CPU won't wait for queue's job to be done and the main thread will go die.*/
	const int32_t dim = (env_cubemap_type == EnvCubemapType::ECT_SPECULAR) ? 512 : 64;
	const uint32_t numMips = static_cast<uint32_t>(floor(std::log2(dim)));
	auto prefilterCube = std::make_shared<Texture>(&vk_contex_);
	prefilterCube->miplevel_size_ = numMips;
	// Pre-filtered cube map
	// Image
	VulkanUtility::CreateImage(
		vk_contex_,
		dim,
		dim,
		numMips,
		true,
		VK_SAMPLE_COUNT_1_BIT,
		format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		prefilterCube->texture_image_,
		prefilterCube->texture_image_memory_);
	//Image View	
	VulkanUtility::CreateImageView(
		vk_contex_, 
		prefilterCube->texture_image_, 
		format, 
		VK_IMAGE_ASPECT_COLOR_BIT, 
		numMips,
		VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE,
		&prefilterCube->texture_image_view_);
	
	// Sampler
	VulkanUtility::CreateTextureSampler(vk_contex_, numMips, &prefilterCube->texture_sampler_);
	prefilterCube->descriptor_.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	prefilterCube->descriptor_.imageView = prefilterCube->texture_image_view_;
	prefilterCube->descriptor_.sampler = prefilterCube->texture_sampler_;

	// FB, Att, RP, Pipe, etc.
	VkAttachmentDescription attDesc = {};
	// Color attachment
	attDesc.format = format;
	attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attDesc.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;//why this will lead to a automatic transition but the initialLayout can't.
	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Renderpass
	VkRenderPassCreateInfo renderPassCI{};
	renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCI.attachmentCount = 1;
	renderPassCI.pAttachments = &attDesc;
	renderPassCI.subpassCount = 1;
	renderPassCI.pSubpasses = &subpassDescription;
	renderPassCI.dependencyCount = 2;
	renderPassCI.pDependencies = dependencies.data();
	VkRenderPass renderpass;
	if (vkCreateRenderPass(vk_contex_.vulkan_device.logical_device, &renderPassCI, nullptr, &renderpass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create RENDER PASS!");
	}
	struct {
		VkImage image;
		VkImageView view;
		VkDeviceMemory memory;
		VkFramebuffer framebuffer;
	} offscreen;

	// Offfscreen framebuffer
	{
		// Color attachment
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.format = format;
		imageCreateInfo.extent.width = dim;
		imageCreateInfo.extent.height = dim;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.imageType = VkImageType::VK_IMAGE_TYPE_2D;
		if (vkCreateImage(vk_contex_.vulkan_device.logical_device, &imageCreateInfo, nullptr, &offscreen.image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create offscreen image!");
		}

		VkMemoryAllocateInfo memAlloc{};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(vk_contex_.vulkan_device.logical_device, offscreen.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vk_contex_.vulkan_device.GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(vk_contex_.vulkan_device.logical_device, &memAlloc, nullptr, &offscreen.memory) != VK_SUCCESS) {
			throw std::runtime_error("failed to Allocate Memory!");
		}


		if (vkBindImageMemory(vk_contex_.vulkan_device.logical_device, offscreen.image, offscreen.memory, 0) != VK_SUCCESS) {
			throw std::runtime_error("failed to Bind Image Memory!");
		}

		VkImageViewCreateInfo colorImageView{};
		colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorImageView.format = format;
		colorImageView.flags = 0;
		colorImageView.subresourceRange = {};
		colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorImageView.subresourceRange.baseMipLevel = 0;
		colorImageView.subresourceRange.levelCount = 1;
		colorImageView.subresourceRange.baseArrayLayer = 0;
		colorImageView.subresourceRange.layerCount = 1;
		colorImageView.image = offscreen.image;

		if (vkCreateImageView(vk_contex_.vulkan_device.logical_device, &colorImageView, nullptr, &offscreen.view) != VK_SUCCESS) {
			throw std::runtime_error("failed to create Image view!");
		}

		VkFramebufferCreateInfo fbufCreateInfo = {};
		fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbufCreateInfo.renderPass = renderpass;
		fbufCreateInfo.attachmentCount = 1;
		fbufCreateInfo.pAttachments = &offscreen.view;
		fbufCreateInfo.width = dim;
		fbufCreateInfo.height = dim;
		fbufCreateInfo.layers = 1;
		if (vkCreateFramebuffer(vk_contex_.vulkan_device.logical_device, &fbufCreateInfo, nullptr, &offscreen.framebuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create Framebuffer!");
		}

		VulkanUtility::TransitionImageLayout(
			vk_contex_, 
			offscreen.image,
			format,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			1,static_cast<uint32_t>(1));
	}

	// Descriptor layout
	VkDescriptorSetLayout descriptorsetlayout;
	VkDescriptorSetLayoutBinding setLayoutBinding{};
	setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	setLayoutBinding.binding = 0;
	setLayoutBinding.descriptorCount = 1;
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {setLayoutBinding};
	VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI{};
	descriptorsetlayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorsetlayoutCI.pBindings = setLayoutBindings.data();
	descriptorsetlayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
	if (vkCreateDescriptorSetLayout(vk_contex_.vulkan_device.logical_device, &descriptorsetlayoutCI, nullptr, &descriptorsetlayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create DescriptorSetLayout!");
	}

	// Descriptor Pool
	VkDescriptorPool descriptorpool;
	VkDescriptorPoolSize descriptorPoolSize{};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSize.descriptorCount = 1;
	std::vector<VkDescriptorPoolSize> poolSizes = { descriptorPoolSize };
	VkDescriptorPoolCreateInfo descriptorPoolCI{};
	descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolCI.pPoolSizes = poolSizes.data();
	descriptorPoolCI.maxSets = static_cast<uint32_t>(2);
	if (vkCreateDescriptorPool(vk_contex_.vulkan_device.logical_device, &descriptorPoolCI, nullptr, &descriptorpool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create Descriptor pool!");
	}

	// Descriptor sets
	VkDescriptorSet descriptorset;
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorpool;
	allocInfo.pSetLayouts = &descriptorsetlayout;
	allocInfo.descriptorSetCount = 1;
	if (vkAllocateDescriptorSets(vk_contex_.vulkan_device.logical_device, &allocInfo, &descriptorset) != VK_SUCCESS) {
		throw std::runtime_error("failed to Allocate DescriptorSets!");
	}
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = descriptorset;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSet.dstBinding = 0;
	writeDescriptorSet.pImageInfo = &TextureMgr::GetInstance().GetTextureByName("sky_texcube")->descriptor_;
	writeDescriptorSet.descriptorCount = 1;

	
	vkUpdateDescriptorSets(vk_contex_.vulkan_device.logical_device, 1, &writeDescriptorSet, 0, nullptr);

	// Pipeline layout
	struct PushBlock {
		glm::mat4 mvp;
		float roughness = 0u;
		uint32_t numSamples = 64u;
	} pushBlock;

	VkPipelineLayout pipelinelayout;
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushBlock);

	std::vector<VkPushConstantRange> pushConstantRanges = { pushConstantRange,};


	VkPipelineLayoutCreateInfo pipelineLayoutCI{};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.pSetLayouts = &descriptorsetlayout;
	pipelineLayoutCI.pushConstantRangeCount = pushConstantRanges.size();
	pipelineLayoutCI.pPushConstantRanges = pushConstantRanges.data();
	if (vkCreatePipelineLayout(vk_contex_.vulkan_device.logical_device, &pipelineLayoutCI, nullptr, &pipelinelayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to Create Pipeline Layout!");
	}

	// Pipeline
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyState.flags = 0;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;
	VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
	rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
	rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCI.flags = 0;
	rasterizationStateCI.depthClampEnable = VK_FALSE;
	rasterizationStateCI.lineWidth = 1.0f;
	

	VkPipelineColorBlendAttachmentState blendAttachmentState{};
	blendAttachmentState.colorWriteMask = 0xf;
	blendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendState{};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &blendAttachmentState;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
	depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCI.depthTestEnable = VK_FALSE;
	depthStencilStateCI.depthWriteEnable = VK_FALSE;
	depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilStateCI.front = depthStencilStateCI.back;
	depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;

	VkPipelineViewportStateCreateInfo viewportStateCI{};
	viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCI.viewportCount = 1;
	viewportStateCI.scissorCount = 1;
	viewportStateCI.flags = 0;

	VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
	multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCI.flags = 0;

	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateCI{};
	dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
	dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicStateCI.flags = 0;

	// Vertex input state
	VkVertexInputBindingDescription vertexInputBinding{};
	vertexInputBinding.binding = 0;
	vertexInputBinding.stride = sizeof(Vertex);
	vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vertexInputAttribute{};
	vertexInputAttribute.location = 0;
	vertexInputAttribute.binding = 0;
	vertexInputAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexInputAttribute.offset = 0;

	VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
	vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCI.vertexBindingDescriptionCount = 1;
	vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
	vertexInputStateCI.vertexAttributeDescriptionCount = 1;
	vertexInputStateCI.pVertexAttributeDescriptions = &vertexInputAttribute;

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkGraphicsPipelineCreateInfo pipelineCI{};
	pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCI.layout = pipelinelayout;
	pipelineCI.renderPass = renderpass;
	pipelineCI.flags = 0;
	pipelineCI.basePipelineIndex = -1;
	pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCI.pInputAssemblyState = &inputAssemblyState;
	pipelineCI.pRasterizationState = &rasterizationStateCI;
	pipelineCI.pColorBlendState = &colorBlendState;
	pipelineCI.pMultisampleState = &multisampleStateCI;
	pipelineCI.pViewportState = &viewportStateCI;
	pipelineCI.pDepthStencilState = &depthStencilStateCI;
	pipelineCI.pDynamicState = &dynamicStateCI;
	pipelineCI.stageCount = 2;
	pipelineCI.pStages = shaderStages.data();
	pipelineCI.pVertexInputState = &vertexInputStateCI;
	pipelineCI.renderPass = renderpass;


	shaderStages[0] = ShaderMgr::GetInstance().LoadShader(vk_contex_, "resource/shaders/compiled_shaders/filtercube_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	if (env_cubemap_type == EnvCubemapType::ECT_SPECULAR)
	{
		shaderStages[1] = ShaderMgr::GetInstance().LoadShader(vk_contex_, "resource/shaders/compiled_shaders/prefilter_specular_envmap_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	else
	{
		shaderStages[1] = ShaderMgr::GetInstance().LoadShader(vk_contex_, "resource/shaders/compiled_shaders/prefilter_diffuse_envmap_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	VkPipeline pipeline;

	if (vkCreateGraphicsPipelines(vk_contex_.vulkan_device.logical_device, vk_contex_.pipeline_cache, 1, &pipelineCI, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to Create Graphics Pipelines!");
	}

	// Render

	VkClearValue clearValues[1];
	clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	// Reuse render pass from example pass
	renderPassBeginInfo.renderPass = renderpass;
	renderPassBeginInfo.framebuffer = offscreen.framebuffer;
	renderPassBeginInfo.renderArea.extent.width = dim;
	renderPassBeginInfo.renderArea.extent.height = dim;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;

	std::vector<glm::mat4> matrices = {
		// POSITIVE_X
		glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_X
		glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// POSITIVE_Y
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_Y
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// POSITIVE_Z
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_Z
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	};

	VkCommandBuffer cmdBuf;
	vk_contex_.vulkan_device.BeginSingleTimeCommands(static_cast<uint32_t>(vk_contex_.vulkan_device.queue_family_indices.graphicsFamily), cmdBuf);
	VkViewport viewport{};
	viewport.width = (float)dim;
	viewport.height = (float)dim;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.extent.width = dim;
	scissor.extent.height = dim;
	scissor.offset.x = 0;
	scissor.offset.y = 0;

	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

	VkImageSubresourceRange subresourceRange_cubemap = {};
	subresourceRange_cubemap.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange_cubemap.baseMipLevel = 0;
	subresourceRange_cubemap.levelCount = numMips;
	subresourceRange_cubemap.layerCount = 6;

	VkImageSubresourceRange subresourceRange_offscreen_rt = {};
	subresourceRange_offscreen_rt.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange_offscreen_rt.baseMipLevel = 0;
	subresourceRange_offscreen_rt.levelCount = 1;
	subresourceRange_offscreen_rt.layerCount = 1;


	// Change image layout for all cubemap faces to transfer destination
	VulkanUtility::SetImageLayout(
		cmdBuf,
		prefilterCube->texture_image_,
		format,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange_cubemap);

	for (uint32_t m = 0; m < numMips; m++) {
		if (env_cubemap_type == EnvCubemapType::ECT_SPECULAR)
		{
			pushBlock.roughness = (float)m / (float)(numMips - 1);
		}
		for (uint32_t f = 0; f < 6; f++) {
			viewport.width = static_cast<float>(dim * std::pow(0.5f, m));
			viewport.height = static_cast<float>(dim * std::pow(0.5f, m));
			vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

			// Render scene from cube face's point of view
			vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Update shader push constant block
			pushBlock.mvp = glm::perspective((float)(M_PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];

			vkCmdPushConstants(cmdBuf, pipelinelayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlock), &pushBlock);

			vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinelayout, 0, 1, &descriptorset, 0, NULL);

			VkDeviceSize offsets[1] = {0};

			vkCmdBindVertexBuffers(cmdBuf, 0, 1, &MeshMgr::GetInstance().GetMeshByName("sky")->vertex_buffer_.buffer, offsets);
			vkCmdBindIndexBuffer(cmdBuf, MeshMgr::GetInstance().GetMeshByName("sky")->index_buffer_.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(cmdBuf, MeshMgr::GetInstance().GetMeshByName("sky")->indices_.size(), 1, 0, 0, 0);
			vkCmdEndRenderPass(cmdBuf);
			// Copy region for transfer from framebuffer to cube face
			VkImageCopy copyRegion = {};

			copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.srcSubresource.baseArrayLayer = 0;
			copyRegion.srcSubresource.mipLevel = 0;
			copyRegion.srcSubresource.layerCount = 1;
			copyRegion.srcOffset = { 0, 0, 0 };

			copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.dstSubresource.baseArrayLayer = f;
			copyRegion.dstSubresource.mipLevel = m;
			copyRegion.dstSubresource.layerCount = 1;
			copyRegion.dstOffset = { 0, 0, 0 };

			copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
			copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
			copyRegion.extent.depth = 1;

			vkCmdCopyImage(
				cmdBuf,
				offscreen.image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				prefilterCube->texture_image_,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&copyRegion);

			// Transform framebuffer color attachment back 
			VulkanUtility::SetImageLayout(
				cmdBuf,
				offscreen.image,
				format,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				subresourceRange_offscreen_rt);
		}
	}


	VulkanUtility::SetImageLayout(
		cmdBuf,
		prefilterCube->texture_image_,
		format,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		subresourceRange_cubemap);
	vk_contex_.vulkan_device.EndSingleTimeCommands(cmdBuf, static_cast<uint32_t>(vk_contex_.vulkan_device.queue_family_indices.graphicsFamily));

	// todo: cleanup
	vkDestroyRenderPass(vk_contex_.vulkan_device.logical_device, renderpass, nullptr);
	vkDestroyFramebuffer(vk_contex_.vulkan_device.logical_device, offscreen.framebuffer, nullptr);
	vkFreeMemory(vk_contex_.vulkan_device.logical_device, offscreen.memory, nullptr);
	vkDestroyImageView(vk_contex_.vulkan_device.logical_device, offscreen.view, nullptr);
	vkDestroyImage(vk_contex_.vulkan_device.logical_device, offscreen.image, nullptr);
	vkDestroyDescriptorPool(vk_contex_.vulkan_device.logical_device, descriptorpool, nullptr);
	vkDestroyDescriptorSetLayout(vk_contex_.vulkan_device.logical_device, descriptorsetlayout, nullptr);
	vkDestroyPipeline(vk_contex_.vulkan_device.logical_device, pipeline, nullptr);
	vkDestroyPipelineLayout(vk_contex_.vulkan_device.logical_device, pipelinelayout, nullptr);

	auto tEnd = std::chrono::high_resolution_clock::now();
	auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
	std::cout << "Generating pre-filtered enivornment cube(" 
		      << ((env_cubemap_type == EnvCubemapType::ECT_SPECULAR)? "specular":"irradiance")
			  <<") with " << numMips << " mip levels took " << tDiff << " ms" << std::endl;
	return prefilterCube;
}

std::shared_ptr<Texture> HakunaRenderer::GenerateBRDFLUT()
{
	auto tStart = std::chrono::high_resolution_clock::now();
	const VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
	/* if the job takes a lot of time, CPU won't wait for queue's job to be done and the main thread will go die.*/
	const int32_t dim = 512;
	const uint32_t numMips = 1;
	auto brdf_lut =std::make_shared<Texture>(&vk_contex_);
	brdf_lut->miplevel_size_ = numMips;
	// Pre-filtered cube map
	// Image
	VulkanUtility::CreateImage(
		vk_contex_,
		dim,
		dim,
		numMips,
		false,
		VK_SAMPLE_COUNT_1_BIT,
		format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		brdf_lut->texture_image_,
		brdf_lut->texture_image_memory_);
	//Image View	
	VulkanUtility::CreateImageView(
		vk_contex_,
		brdf_lut->texture_image_,
		format,
		VK_IMAGE_ASPECT_COLOR_BIT,
		numMips,
		VkImageViewType::VK_IMAGE_VIEW_TYPE_2D,
		&brdf_lut->texture_image_view_);

	// Sampler
	VulkanUtility::CreateTextureSampler(vk_contex_, numMips, &brdf_lut->texture_sampler_);
	brdf_lut->descriptor_.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	brdf_lut->descriptor_.imageView = brdf_lut->texture_image_view_;
	brdf_lut->descriptor_.sampler = brdf_lut->texture_sampler_;


	// FB, Att, RP, Pipe, etc.
	VkAttachmentDescription attDesc = {};
	// Color attachment
	attDesc.format = format;
	attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;//
	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Renderpass
	VkRenderPassCreateInfo renderPassCI{};
	renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCI.attachmentCount = 1;
	renderPassCI.pAttachments = &attDesc;
	renderPassCI.subpassCount = 1;
	renderPassCI.pSubpasses = &subpassDescription;
	renderPassCI.dependencyCount = 2;
	renderPassCI.pDependencies = dependencies.data();
	VkRenderPass renderpass;
	if (vkCreateRenderPass(vk_contex_.vulkan_device.logical_device, &renderPassCI, nullptr, &renderpass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create RENDER PASS!");
	}
	

	VkFramebufferCreateInfo fbufCreateInfo = {};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.renderPass = renderpass;
	fbufCreateInfo.attachmentCount = 1;
	fbufCreateInfo.pAttachments = &brdf_lut->texture_image_view_;
	fbufCreateInfo.width = dim;
	fbufCreateInfo.height = dim;
	fbufCreateInfo.layers = 1;
	VkFramebuffer framebuffer;
	if (vkCreateFramebuffer(vk_contex_.vulkan_device.logical_device, &fbufCreateInfo, nullptr, &framebuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create Framebuffer!");
	}

	// Descriptor layout
	VkDescriptorSetLayout descriptorsetlayout;
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};
	VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI{};
	descriptorsetlayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorsetlayoutCI.pBindings = setLayoutBindings.data();
	descriptorsetlayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
	if (vkCreateDescriptorSetLayout(vk_contex_.vulkan_device.logical_device, &descriptorsetlayoutCI, nullptr, &descriptorsetlayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create DescriptorSetLayout!");
	}

	// Descriptor Pool
	VkDescriptorPool descriptorpool;
	VkDescriptorPoolSize descriptorPoolSize{};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSize.descriptorCount = 1;
	std::vector<VkDescriptorPoolSize> poolSizes = { descriptorPoolSize };
	VkDescriptorPoolCreateInfo descriptorPoolCI{};
	descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolCI.pPoolSizes = poolSizes.data();
	descriptorPoolCI.maxSets = static_cast<uint32_t>(2);
	if (vkCreateDescriptorPool(vk_contex_.vulkan_device.logical_device, &descriptorPoolCI, nullptr, &descriptorpool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create Descriptor pool!");
	}

	// Descriptor sets
	VkDescriptorSet descriptorset;
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorpool;
	allocInfo.pSetLayouts = &descriptorsetlayout;
	allocInfo.descriptorSetCount = 1;
	if (vkAllocateDescriptorSets(vk_contex_.vulkan_device.logical_device, &allocInfo, &descriptorset) != VK_SUCCESS) {
		throw std::runtime_error("failed to Allocate DescriptorSets!");
	}


	VkPipelineLayout pipelinelayout;
	VkPipelineLayoutCreateInfo pipelineLayoutCI{};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.pSetLayouts = &descriptorsetlayout;
	if (vkCreatePipelineLayout(vk_contex_.vulkan_device.logical_device, &pipelineLayoutCI, nullptr, &pipelinelayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to Create Pipeline Layout!");
	}

	// Pipeline
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyState.flags = 0;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;
	VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
	rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
	rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCI.flags = 0;
	rasterizationStateCI.depthClampEnable = VK_FALSE;
	rasterizationStateCI.lineWidth = 1.0f;


	VkPipelineColorBlendAttachmentState blendAttachmentState{};
	blendAttachmentState.colorWriteMask = 0xf;
	blendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendState{};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &blendAttachmentState;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
	depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCI.depthTestEnable = VK_FALSE;
	depthStencilStateCI.depthWriteEnable = VK_FALSE;
	depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilStateCI.front = depthStencilStateCI.back;
	depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;

	VkPipelineViewportStateCreateInfo viewportStateCI{};
	viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCI.viewportCount = 1;
	viewportStateCI.scissorCount = 1;
	viewportStateCI.flags = 0;

	VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
	multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCI.flags = 0;

	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateCI{};
	dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
	dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicStateCI.flags = 0;
	VkPipelineVertexInputStateCreateInfo emptyVertexInputStateCI{};
	emptyVertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkGraphicsPipelineCreateInfo pipelineCI{};
	pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCI.layout = pipelinelayout;
	pipelineCI.renderPass = renderpass;
	pipelineCI.flags = 0;
	pipelineCI.basePipelineIndex = -1;
	pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCI.pInputAssemblyState = &inputAssemblyState;
	pipelineCI.pRasterizationState = &rasterizationStateCI;
	pipelineCI.pColorBlendState = &colorBlendState;
	pipelineCI.pMultisampleState = &multisampleStateCI;
	pipelineCI.pViewportState = &viewportStateCI;
	pipelineCI.pDepthStencilState = &depthStencilStateCI;
	pipelineCI.pDynamicState = &dynamicStateCI;
	pipelineCI.stageCount = 2;
	pipelineCI.pStages = shaderStages.data();
	pipelineCI.pVertexInputState = &emptyVertexInputStateCI;
	pipelineCI.renderPass = renderpass;
	shaderStages[0] = ShaderMgr::GetInstance().LoadShader(vk_contex_, "resource/shaders/compiled_shaders/gen_brdf_lut_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = ShaderMgr::GetInstance().LoadShader(vk_contex_, "resource/shaders/compiled_shaders/gen_brdf_lut_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkPipeline pipeline;
	if (vkCreateGraphicsPipelines(vk_contex_.vulkan_device.logical_device, vk_contex_.pipeline_cache, 1, &pipelineCI, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to Create Graphics Pipelines!");
	}

	// Render

	VkClearValue clearValues[1];
	clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	// Reuse render pass from example pass
	renderPassBeginInfo.renderPass = renderpass;
	renderPassBeginInfo.framebuffer = framebuffer;
	renderPassBeginInfo.renderArea.extent.width = dim;
	renderPassBeginInfo.renderArea.extent.height = dim;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;

	VkCommandBuffer cmdBuf;
	vk_contex_.vulkan_device.BeginSingleTimeCommands(static_cast<uint32_t>(vk_contex_.vulkan_device.queue_family_indices.graphicsFamily), cmdBuf);
	// Render scene from cube face's point of view
	vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	VkViewport viewport{};
	viewport.width = (float)dim;
	viewport.height = (float)dim;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{};
	scissor.extent.width = dim;
	scissor.extent.height = dim;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);		
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdDraw(cmdBuf, 3, 1, 0, 0);
	vkCmdEndRenderPass(cmdBuf);
	vk_contex_.vulkan_device.EndSingleTimeCommands(cmdBuf, static_cast<uint32_t>(vk_contex_.vulkan_device.queue_family_indices.graphicsFamily));

	// todo: cleanup
	vkDestroyRenderPass(vk_contex_.vulkan_device.logical_device, renderpass, nullptr);
	vkDestroyFramebuffer(vk_contex_.vulkan_device.logical_device, framebuffer, nullptr);
	vkDestroyDescriptorPool(vk_contex_.vulkan_device.logical_device, descriptorpool, nullptr);
	vkDestroyDescriptorSetLayout(vk_contex_.vulkan_device.logical_device, descriptorsetlayout, nullptr);
	vkDestroyPipeline(vk_contex_.vulkan_device.logical_device, pipeline, nullptr);
	vkDestroyPipelineLayout(vk_contex_.vulkan_device.logical_device, pipelinelayout, nullptr);

	auto tEnd = std::chrono::high_resolution_clock::now();
	auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
	std::cout << "Generating brdf lut("<< ") with " << numMips << " mip levels took " << tDiff << " ms" << std::endl;
	return brdf_lut;
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