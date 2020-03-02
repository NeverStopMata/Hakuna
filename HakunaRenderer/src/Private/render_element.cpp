#include "..\Public\render_element.h"
#include "..\Public\HakunaRenderer.h"
RenderElement::RenderElement(string name, Transform transform, HakunaRenderer* ptr_renderer, ERenderElementTtype occlusion_type):ptr_renderer_(ptr_renderer), occlusion_type_(occlusion_type)
{
	ptr_material_ = make_shared<Material>(name);
	MeshMgr::GetInstance()->LoadModelFromFile(mesh_resource_path_ + name + mesh_postfix_, name);
	for(auto texture_type : this->ptr_material_->texture_types_)
	{
		if (!TextureMgr::GetInstance()->FindTextureByName(name + "_" + texture_type))
		{
			TextureMgr::GetInstance()->AddTexture(name +"_"+ texture_type, (new Texture(&(ptr_renderer->vk_contex_)))->LoadTexture2D(texture_resource_path_ + name +"/"+texture_type + texture_postfix_));
		}
	}

	ptr_mesh_ = MeshMgr::GetInstance()->GetMeshByName(name);
	ptr_bbox_obj_space_ = ptr_mesh_->ptr_bbox_;

	model_transform_ = transform;
	UpdateWorldSpaceAABB();
	InitOclusionInfo();
	CreateDescriptorSets();
}
void RenderElement::CommitRendering(uint32_t currentImage)
{
	if (!this->NeedRender()) { return; }
	vkCmdPushConstants(
		ptr_renderer_->command_buffers_[currentImage],
		ptr_renderer_->vk_contex_.pipeline_layout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		model_transform_.GetMaticesSize(),
		model_transform_.GetModelMatricesBeginPtr());
	VkBuffer vertexBuffers[] = { ptr_mesh_->vertex_buffer_.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindDescriptorSets(ptr_renderer_->command_buffers_[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, ptr_renderer_->vk_contex_.pipeline_layout, 0, 1, &descriptor_sets_[currentImage], 0, nullptr);
	vkCmdBindVertexBuffers(ptr_renderer_->command_buffers_[currentImage], 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(ptr_renderer_->command_buffers_[currentImage], ptr_mesh_->index_buffer_.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(ptr_renderer_->command_buffers_[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, ptr_renderer_->vk_contex_.pipeline_struct.opaque_pipeline);
	vkCmdDrawIndexed(ptr_renderer_->command_buffers_[currentImage], static_cast<uint32_t>(ptr_mesh_->indices_.size()), 1, 0, 0, 0);
}

void RenderElement::UpdateTransform(const Transform& new_transform, uint32_t currentImage)
{
	this->model_transform_.UpdateTransform(new_transform);
}

void RenderElement::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(ptr_renderer_->vk_contex_.vulkan_swapchain.imageCount_, ptr_renderer_->vk_contex_.descriptor_set_layout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = ptr_renderer_->vk_contex_.descriptor_pool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(ptr_renderer_->vk_contex_.vulkan_swapchain.imageCount_);
	allocInfo.pSetLayouts = layouts.data();
	descriptor_sets_.resize(ptr_renderer_->vk_contex_.vulkan_swapchain.imageCount_);
	if (vkAllocateDescriptorSets(ptr_renderer_->vk_contex_.vulkan_device.logical_device, &allocInfo, descriptor_sets_.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
	for (size_t i = 0; i < ptr_renderer_->vk_contex_.vulkan_swapchain.imageCount_; i++) {
		VkDescriptorBufferInfo global_params_buffer_Info = {};
		global_params_buffer_Info.buffer = ptr_renderer_->global_params_ubos_[i].buffer;
		global_params_buffer_Info.offset = 0;
		global_params_buffer_Info.range = sizeof(UboGlobalParams);

		std::vector<VkWriteDescriptorSet> descriptorWrites(1 + ptr_material_->material_tex_names_.size());

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptor_sets_[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &global_params_buffer_Info;

		for (int k = 1; k < descriptorWrites.size(); k++) {
			descriptorWrites[k].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[k].dstSet = descriptor_sets_[i];
			descriptorWrites[k].dstBinding = k;
			descriptorWrites[k].dstArrayElement = 0;
			descriptorWrites[k].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[k].descriptorCount = 1;
			descriptorWrites[k].pImageInfo = &TextureMgr::GetInstance()->GetTextureByName(ptr_material_->material_tex_names_[k - 1])->descriptor_;
		}
		vkUpdateDescriptorSets(ptr_renderer_->vk_contex_.vulkan_device.logical_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void RenderElement::InitOclusionInfo()
{
	if (occlusion_type_ == ERenderElementTtype::OccluderOnly || occlusion_type_ == ERenderElementTtype::OccluderAndOccludee)
	{
		this->ptr_mesh_->GetAllTriangles(occluder_triangles_);
	}
}

void RenderElement::UpdateWorldSpaceAABB()
{
	//8*32 256 
	glm::mat4 original_model_matrix = this->GetModelMatrix<false>();
	__m256 model_matrix[16];
	for (int matrix_ele_idx = 0; matrix_ele_idx < 16; ++matrix_ele_idx)
	{
		/*
		0  4  8  12
		1  5  9  13
		2  6  10 14
		3  7  11 15
		*/
		model_matrix[matrix_ele_idx] = _mm256_set1_ps(original_model_matrix[matrix_ele_idx / 4][matrix_ele_idx % 4]);
	}
	__m256 obj_pos[4];//0 1 2 3->x y z w
	for (int lane_idx = 0; lane_idx < 8; lane_idx++)
	{
		for (int dim = 0; dim < 3; dim++)
		{
			simd_f32(obj_pos[dim])[lane_idx] = (lane_idx & (1<<dim))? this->ptr_bbox_obj_space_->GetMax()[dim]: this->ptr_bbox_obj_space_->GetMin()[dim];
		}
		obj_pos[3] = _mm256_set1_ps(1.0f);
	}
	__m256 world_pos[3];//0 1 2 ->x y z
	for (int dim = 0; dim < 3; dim++)//object-space to clip-space.
	{
		world_pos[dim] = _mm256_fmadd_ps(obj_pos[0], model_matrix[0 * 4 + dim], _mm256_fmadd_ps(obj_pos[1], model_matrix[1 * 4 + dim], _mm256_fmadd_ps(obj_pos[2], model_matrix[2 * 4 + dim], model_matrix[3 * 4 + dim])));
	}
	glm::vec3 max_res = glm::vec3(-std::numeric_limits<float>::max());
	glm::vec3 min_res = glm::vec3(+std::numeric_limits<float>::max());
	for (int lane_idx = 0; lane_idx < 8; lane_idx++)
	{
		glm::vec3 temp_pos = glm::vec3(simd_f32(world_pos[0])[lane_idx], simd_f32(world_pos[1])[lane_idx], simd_f32(world_pos[2])[lane_idx]);
		min_res = glm::min(min_res, temp_pos);
		max_res = glm::max(max_res, temp_pos);
	}
	this->aabb_world_space_.SetMax(max_res);
	this->aabb_world_space_.SetMin(min_res);
}
