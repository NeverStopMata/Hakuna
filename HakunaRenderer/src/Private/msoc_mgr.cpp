#include "msoc_mgr.h"
#include "render_element.h"
#include "HakunaRenderer.h"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <algorithm>
class Cullable;
MSOCManager* MSOCManager::p_instance_ = nullptr;
void MSOCManager::InitMSOCManager(HakunaRenderer* render_ptr)
{
	ptr_renderer_ = render_ptr;
	render_ptr->GetWindowSize(&width_,&height_);
	tiles_width_ = width_ / TILE_WIDTH;
	tiles_height_ = height_ / TILE_HEIGHT;
	masked_hiz_buffer_ = new ZTile[tiles_width_* tiles_height_];
}
void MSOCManager::InitMSOCTriangleLists()
{

	for (int render_ele_idx = 0; render_ele_idx < ptr_renderer_->render_elements_.size(); render_ele_idx++)
	{
		auto render_element = ptr_renderer_->render_elements_[render_ele_idx];
		tri_list_.num_tri += render_element.occluder_triangles_.size();
	}
	tri_list_.ptr_tri = new float[tri_list_.num_tri * 3 * 3];
	
	is_tri_list_initialized = true;
}
void MSOCManager::CollectOccluderTriangles()
{
	if (!is_tri_list_initialized)
	{
		InitMSOCTriangleLists();
	}
	glm::mat4x4 currentmvp;
	auto V = ptr_renderer_->cam_.GetViewMatrix();
	auto P = ptr_renderer_->cam_.GetProjMatrix();
	glm::mat4x4 VP = P * V;
	uint32_t val_idx = 0;
	for (int i = 0; i < ptr_renderer_->render_elements_.size(); i++)
	{
		if (ptr_renderer_->render_elements_[i].IsNeedRender())
		{
			currentmvp = VP * ptr_renderer_->render_elements_[i].GetModelMatrix<false>();
			auto render_element = ptr_renderer_->render_elements_[i];

			//8*32 256 
			__m256 mvp_matrix[16];
			for (int matrix_ele_idx = 0; matrix_ele_idx < 16; ++matrix_ele_idx)
			{
				/*
				0  4  8  12
				1  5  9  13
				2  6  10 14
				3  7  11 15
				*/
				mvp_matrix[matrix_ele_idx] = _mm256_set1_ps(currentmvp[matrix_ele_idx / 4][matrix_ele_idx % 4]);
			}
			uint32_t num_lane = 8;
			uint32_t num_vertex = render_element.occluder_triangles_.size() * 3;
			for (uint32_t vert_begin_idx = 0; vert_begin_idx < render_element.occluder_triangles_.size() * 3; vert_begin_idx += num_lane)
			{
				uint32_t vert_end_idx = std::min(vert_begin_idx + 8, num_vertex);
				__m256 obj_pos[3];//0x 1y 2z
				__m256 ndc_pos[4];//0x 1y 2z 3w
				for (uint32_t vert_idx2 = vert_begin_idx; vert_idx2 < vert_end_idx; vert_idx2++)// pack eight vertices' obj pos into __m256
				{
					uint32_t tri_idx = vert_idx2 / 3;
					uint32_t intri_vert_idx = vert_idx2 % 3;
					uint32_t lane_idx = vert_idx2 % num_lane;
					glm::vec3 pos = render_element.occluder_triangles_[tri_idx].positions[intri_vert_idx];
					simd_f32(obj_pos[0])[lane_idx] = pos.x;
					simd_f32(obj_pos[1])[lane_idx] = pos.y;
					simd_f32(obj_pos[2])[lane_idx] = pos.z;
				}
				for (int dim = 0; dim < 4; dim++)//object-space to clip-space.
				{
					ndc_pos[dim] = _mm256_fmadd_ps(obj_pos[0], mvp_matrix[0 * 4 + dim], _mm256_fmadd_ps(obj_pos[1], mvp_matrix[1 * 4 + dim], _mm256_fmadd_ps(obj_pos[2], mvp_matrix[2 * 4 + dim], mvp_matrix[3 * 4 + dim])));
				}
				for (int dim = 0; dim < 3; dim++)//perspective division.
				{
					ndc_pos[dim] = _mm256_div_ps(ndc_pos[dim], ndc_pos[3]);
				}
				for (uint32_t i = vert_begin_idx; i < vert_end_idx; i++)//write back the result into the array.
				{
					tri_list_.ptr_tri[val_idx++] = simd_f32(ndc_pos[0])[i % 8];
					tri_list_.ptr_tri[val_idx++] = simd_f32(ndc_pos[1])[i % 8];
					tri_list_.ptr_tri[val_idx++] = simd_f32(ndc_pos[2])[i % 8];
				}

			}
		}
	}
	tri_list_.num_tri = val_idx / 9;
}
