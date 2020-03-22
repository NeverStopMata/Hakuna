#include "msoc_mgr.h"
#include "render_element.h"
#include "HakunaRenderer.h"
#include "stb_image_write.h"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <algorithm>
#include <limits.h>
#include <sstream>
#include <string>
#include <vector>
#include "bbox.h"
class Cullable;
MSOCManager* MSOCManager::p_instance_ = nullptr;
void MSOCManager::InitMSOCManager(HakunaRenderer* render_ptr)
{
	ptr_renderer_ = render_ptr;
	UpdateWindowSizes();
}
void MSOCManager::RenderTrilist(const ScissorRect& scissor)
{
	assert(masked_hiz_buffer_ != nullptr);
	for (int i = 0; i < tiles_width_ * tiles_height_; ++i)
	{
		masked_hiz_buffer_[i].mask = _mm256_set1_epi32(0);
		masked_hiz_buffer_[i].z_min[0] = _mm256_set1_ps(0.0f);
		masked_hiz_buffer_[i].z_min[1] = masked_hiz_buffer_[i].z_min[0];
	}
	for (unsigned int i = 0; i < final_tri_list_.num_tri; i += SIMD_LANES)
	{
		//////////////////////////////////////////////////////////////////////////////
		// Fetch triangle vertices
		//////////////////////////////////////////////////////////////////////////////
		unsigned int numLanes = std::min((unsigned int)SIMD_LANES, final_tri_list_.num_tri - i);
		unsigned int triMask = (1U << numLanes) - 1;

		__m256 pVtxX[3], pVtxY[3], pVtxZ[3];

		for (unsigned int l = 0; l < numLanes; ++l)
		{
			unsigned int triIdx = i + l;
			for (int v = 0; v < 3; ++v)
			{
				simd_f32(pVtxX[v])[l] = final_tri_list_.ptr_tri[v * 3 + triIdx * 9 + 0];
				simd_f32(pVtxY[v])[l] = final_tri_list_.ptr_tri[v * 3 + triIdx * 9 + 1];
				simd_f32(pVtxZ[v])[l] = final_tri_list_.ptr_tri[v * 3 + triIdx * 9 + 2];
			}
		}
		//////////////////////////////////////////////////////////////////////////////
		// Setup and rasterize a SIMD batch of triangles
		//////////////////////////////////////////////////////////////////////////////
		RasterizeTriangleBatch<false>(pVtxX, pVtxY, pVtxZ, triMask, scissor);
	}
}
void MSOCManager::TestDepthForAABBoxes()
{
	std::vector< std::future<bool> > results;
	auto V = ptr_renderer_->cam_.GetViewMatrix();
	auto P = ptr_renderer_->cam_.GetProjMatrix();
	glm::mat4x4 VP = P * V;
	__m128 mm_VP_mat[4];
	for (int i = 0; i < 4; i++)
	{
		mm_VP_mat[i] = _mm_loadu_ps(&VP[i][0]);
	}
	for (int i = 0; i < ptr_renderer_->render_elements_.size(); i++)
	{
		RenderElement& render_element = ptr_renderer_->render_elements_[i];
		if (render_element.IsNotOutOfViewFrustum() && render_element.occlusion_type_ != ERenderElementTtype::OccluderOnly)
		{
			results.emplace_back(
				ptr_renderer_->thread_pool_.enqueue([this, mm_VP_mat, &render_element]() {
				return TestPerAABBox(mm_VP_mat, render_element);
			})
			);
		}
	}
	for (auto&& result : results)
	{
		result.get();
	}
}
MSOCManager::~MSOCManager()
{
	PrintZBufferToPicture();
	occludee_bboxes_.clear();
	delete raw_tri_list_.ptr_tri;
	delete final_tri_list_.ptr_tri;
	delete[] masked_hiz_buffer_;
}
void MSOCManager::UpdateWindowSizes()
{
	int old_tiles_width_ = tiles_width_;
	int old_tiles_height_ = tiles_height_;
	ptr_renderer_->GetWindowSize(&width_, &height_);
	half_width_ = static_cast<int>(ceilf(static_cast<float>(width_) / 2.f));
	half_height_ = static_cast<int>(ceilf(static_cast<float>(height_) / 2.f));
	mm_half_width_ = _mm256_set1_ps(static_cast<float>(width_) * 0.5f);
	mm_half_height_ = _mm256_set1_ps(static_cast<float>(height_) * 0.5f);

	m_half_size_ = _mm_setr_ps(half_width_, half_width_, half_height_, half_height_);
	m_screen_size_ = _mm_setr_epi32(width_ - 1, width_ - 1, height_ - 1, height_ - 1);
	mm_center_x_ = mm_half_width_;
	mm_center_y_ = mm_half_height_;
	tiles_width_ = static_cast<int>(floorf(static_cast<float>(width_) / static_cast<float>(TILE_WIDTH)));
	tiles_height_ = static_cast<int>(ceilf(static_cast<float>(height_) / static_cast<float>(TILE_HEIGHT)));
	if (masked_hiz_buffer_ == nullptr)
	{
		masked_hiz_buffer_ = new ZTile[tiles_width_ * tiles_height_];
	}
	else if(old_tiles_width_ * old_tiles_height_ < tiles_width_ * tiles_height_)
	{
		delete[] masked_hiz_buffer_;
		masked_hiz_buffer_ = new ZTile[tiles_width_ * tiles_height_];
	}
	
	for (int i = 0; i < tiles_width_ * tiles_height_; ++i)
	{
		masked_hiz_buffer_[i].mask = _mm256_set1_epi32(0);
		masked_hiz_buffer_[i].z_min[0] = _mm256_set1_ps(0.f);
		masked_hiz_buffer_[i].z_min[1] = masked_hiz_buffer_[i].z_min[0];
	}
	mm_frustum_planes_[0] = _mm_setr_ps(0.0f, +0.0f, 1.0f, 0.0f);
	mm_frustum_planes_[1] = _mm_setr_ps(0.0f, -1.0f, 1.0f, 0.0f);
	mm_frustum_planes_[2] = _mm_setr_ps(0.0f, +1.0f, 1.0f, 0.0f);
	mm_frustum_planes_[3] = _mm_setr_ps(-1.0f, 0.0f, 1.0f, 0.0f);
	mm_frustum_planes_[4] = _mm_setr_ps(+1.0f, 0.0f, 1.0f, 0.0f);
}
bool MSOCManager::TestPerAABBox(const __m128 mm_VP_mat[4], RenderElement& render_element)
{
	const Bbox& aabb_world_space = render_element.GetAABBInWorldSpace();
	glm::vec3 max = aabb_world_space.GetMax();
	glm::vec3 min = aabb_world_space.GetMin();

	// w ends up being garbage, but it doesn't matter - we ignore it anyway.
	__m128 vMin = _mm_loadu_ps(&min.x);
	__m128 vMax = _mm_loadu_ps(&max.x);

	// transforms
	__m128 xCol[2], yCol[2], zCol[2];

	xCol[0] = _mm_mul_ps(_mm_shuffle_ps(vMin, vMin, 0x00), mm_VP_mat[0]);
	xCol[1] = _mm_mul_ps(_mm_shuffle_ps(vMax, vMax, 0x00), mm_VP_mat[0]);
	yCol[0] = _mm_mul_ps(_mm_shuffle_ps(vMin, vMin, 0x55), mm_VP_mat[1]);
	yCol[1] = _mm_mul_ps(_mm_shuffle_ps(vMax, vMax, 0x55), mm_VP_mat[1]);
	zCol[0] = _mm_mul_ps(_mm_shuffle_ps(vMin, vMin, 0xaa), mm_VP_mat[2]);
	zCol[1] = _mm_mul_ps(_mm_shuffle_ps(vMax, vMax, 0xaa), mm_VP_mat[2]);

	__m128 zAllIn = _mm_castsi128_ps(_mm_set1_epi32(~0));
	__m128 ndc_min = _mm_set1_ps(FLT_MAX);
	__m128 ndc_max = _mm_set1_ps(-FLT_MAX);

	// Find the minimum of each component
	__m128 minvert = _mm_add_ps(mm_VP_mat[3], _mm_add_ps(_mm_add_ps(_mm_min_ps(xCol[0], xCol[1]), _mm_min_ps(yCol[0], yCol[1])), _mm_min_ps(zCol[0], zCol[1])));
	float minW = minvert.m128_f32[3];
	if (minW < 0.000000001f)
	{
		render_element.SetIsOccluded(false);
		return true;
	}
	__m128 ndc_pos[8];
	for (int i = 0; i < 8; i++)
	{
		// Transform the vertex
		__m128 vert = mm_VP_mat[3];
		vert = _mm_add_ps(vert, xCol[sBBxInd[i]]);
		vert = _mm_add_ps(vert, yCol[sBByInd[i]]);
		vert = _mm_add_ps(vert, zCol[sBBzInd[i]]);

		// We have inverted z; z is in front of near plane iff z <= w.
		//__m128 vertZ = _mm_shuffle_ps(vert, vert, 0xaa); // vert.zzzz
		__m128 vertW = _mm_shuffle_ps(vert, vert, 0xff); // vert.wwww

		// project
		ndc_pos[i] = _mm_div_ps(vert, vertW);

		// update bounds
		ndc_min = _mm_min_ps(ndc_min, ndc_pos[i]);
		ndc_max = _mm_max_ps(ndc_max, ndc_pos[i]);
	}
	render_element.SetIsOccluded(TestRect(ndc_min.m128_f32[0], ndc_min.m128_f32[1], ndc_max.m128_f32[0], ndc_max.m128_f32[1], minW) != ECullingResult::VISIBLE);
	return true;
}
ECullingResult MSOCManager::TestRect(float xmin, float ymin, float xmax, float ymax, float wmin) const
{
	assert(masked_hiz_buffer_ != nullptr);
	static const __m128i SIMD_TILE_PAD = _mm_setr_epi32(0, TILE_WIDTH, 0, TILE_HEIGHT);
	static const __m128i SIMD_TILE_PAD_MASK = _mm_setr_epi32(~(TILE_WIDTH - 1), ~(TILE_WIDTH - 1), ~(TILE_HEIGHT - 1), ~(TILE_HEIGHT - 1));
	static const __m128i SIMD_SUB_TILE_PAD = _mm_setr_epi32(0, SUB_TILE_WIDTH, 0, SUB_TILE_HEIGHT);
	static const __m128i SIMD_SUB_TILE_PAD_MASK = _mm_setr_epi32(~(SUB_TILE_WIDTH - 1), ~(SUB_TILE_WIDTH - 1), ~(SUB_TILE_HEIGHT - 1), ~(SUB_TILE_HEIGHT - 1));

	//		//////////////////////////////////////////////////////////////////////////////
	//		// Compute screen space bounding box and guard for out of bounds
	//		//////////////////////////////////////////////////////////////////////////////
#if USE_D3D != 0
	__m128  pixelBBox = _mmx_fmadd_ps(_mm_setr_ps(xmin, xmax, ymax, ymin), mIHalfSize, mICenter);
#else
	__m128  pixelBBox = _mm_fmadd_ps(_mm_setr_ps(xmin, xmax, ymin, ymax), m_half_size_, m_half_size_);
#endif
	__m128i pixelBBoxi = _mm_cvttps_epi32(pixelBBox);
	pixelBBoxi = _mm_max_epi32(_mm_setzero_si128(), _mm_min_epi32(m_screen_size_, pixelBBoxi));

	//////////////////////////////////////////////////////////////////////////////
	// Pad bounding box to (32xN) tiles. Tile BB is used for looping / traversal
	//////////////////////////////////////////////////////////////////////////////
	__m128i tileBBoxi = _mm_and_si128(_mm_add_epi32(pixelBBoxi, SIMD_TILE_PAD), SIMD_TILE_PAD_MASK);
	int tiles_x_min = simd_i32(tileBBoxi)[0] >> TILE_WIDTH_SHIFT;
	int tiles_x_max = simd_i32(tileBBoxi)[1] >> TILE_WIDTH_SHIFT;
	int tilesRowIdx = (simd_i32(tileBBoxi)[2] >> TILE_HEIGHT_SHIFT)* tiles_width_;
	int tilesRowIdxEnd = (simd_i32(tileBBoxi)[3] >> TILE_HEIGHT_SHIFT)* tiles_width_;
	//
	if (simd_i32(tileBBoxi)[0] == simd_i32(tileBBoxi)[1] || simd_i32(tileBBoxi)[2] == simd_i32(tileBBoxi)[3])
	{
		return ECullingResult::VIEW_CULLED;
	}
	///////////////////////////////////////////////////////////////////////////////
// Pad bounding box to (8x4) subtiles. Skip SIMD lanes outside the subtile BB
///////////////////////////////////////////////////////////////////////////////
	__m128i subTileBBoxi = _mm_and_si128(_mm_add_epi32(pixelBBoxi, SIMD_SUB_TILE_PAD), SIMD_SUB_TILE_PAD_MASK);//与SUB tile 对齐 x:0,8,16... y:4,8,12......
	__m256i stxmin = _mm256_set1_epi32(simd_i32(subTileBBoxi)[0] - 1); // - 1 to be able to use GT test
	__m256i stymin = _mm256_set1_epi32(simd_i32(subTileBBoxi)[2] - 1); // - 1 to be able to use GT test
	__m256i stxmax = _mm256_set1_epi32(simd_i32(subTileBBoxi)[1]);
	__m256i stymax = _mm256_set1_epi32(simd_i32(subTileBBoxi)[3]);

	// Setup pixel coordinates used to discard lanes outside subtile BB
	__m256i startPixelX = _mm256_add_epi32(SIMD_SUB_TILE_COL_OFFSET, _mm256_set1_epi32(simd_i32(tileBBoxi)[0]));
	__m256i pixelY = _mm256_add_epi32(SIMD_SUB_TILE_ROW_OFFSET, _mm256_set1_epi32(simd_i32(tileBBoxi)[2]));

	//////////////////////////////////////////////////////////////////////////////
	// Compute z from w. Note that z is reversed order, 0 = far, 1 = near, which
	// means we use a greater than test, so zMax is used to test for visibility.
	//////////////////////////////////////////////////////////////////////////////
	__m256 zMax = _mm256_div_ps(_mm256_set1_ps(1.0f), _mm256_set1_ps(wmin));

	for (;;)
	{
		__m256i pixelX = startPixelX;
		for (int tx = tiles_x_min;;)
		{
			int tileIdx = tilesRowIdx + tx;
			assert(tileIdx >= 0 && tileIdx < tiles_width_ * tiles_height_);
			// Fetch zMin from masked hierarchical Z buffer
			__m256 zBuf = masked_hiz_buffer_[tileIdx].z_min[0];

			// Perform conservative greater than test against hierarchical Z buffer (zMax >= zBuf means the subtile is visible)
			__m256i zPass = simd_cast<__m256i>(_mm256_cmpge_ps(zMax, zBuf));	//zPass = zMax >= zBuf ? ~0 : 0

			// Mask out lanes corresponding to subtiles outside the bounding box
			__m256i bboxTestMin = _mm256_and_si256(_mm256_cmpgt_epi32(pixelX, stxmin), _mm256_cmpgt_epi32(pixelY, stymin));
			__m256i bboxTestMax = _mm256_and_si256(_mm256_cmpgt_epi32(stxmax, pixelX), _mm256_cmpgt_epi32(stymax, pixelY));
			__m256i boxMask = _mm256_and_si256(bboxTestMin, bboxTestMax);
			zPass = _mm256_and_si256(zPass, boxMask);

			// If not all tiles failed the conservative z test we can immediately terminate the test
			if (!_mm256_testz_si256(zPass, zPass))//可判断是否全为0
			{
				return ECullingResult::VISIBLE;
			}

			if (++tx >= tiles_x_max)
				break;
			pixelX = _mm256_add_epi32(pixelX, _mm256_set1_epi32(TILE_WIDTH));
		}

		tilesRowIdx += tiles_width_;
		if (tilesRowIdx >= tilesRowIdxEnd)
			break;
		pixelY = _mm256_add_epi32(pixelY, _mm256_set1_epi32(TILE_HEIGHT));
	}
	return ECullingResult::OCCLUDED;
}
void MSOCManager::InitMSOCTriangleLists()
{
	int total_triangle_num = 0;
	for (int render_ele_idx = 0; render_ele_idx < ptr_renderer_->render_elements_.size(); render_ele_idx++)
	{
		auto render_element = ptr_renderer_->render_elements_[render_ele_idx];
		total_triangle_num += render_element.occluder_triangles_.size();
	}
	raw_tri_list_.ptr_tri = new float[total_triangle_num * 3 * 3];
	final_tri_list_.ptr_tri = new float[total_triangle_num * 3 * 3];
	is_tri_list_initialized = true;
}
void MSOCManager::PrintZBufferToPicture()
{
	std::vector<char> result_buffer(width_ * height_ * 3);
	std::vector<float> imm_result_buffer(width_ * height_);
	std::stringstream file_name;
	file_name << "test_res.png";
	uint32 write_idx = 0;
	float depth_max = -std::numeric_limits<float>::max();
	for (int row = height_ - 1; row >= 0; row--)
	{
		uint32 tile_idx_y = row / TILE_HEIGHT;
		uint32 intile_idx_y = row % TILE_HEIGHT;
		for (int col = 0; col < width_; col++)
		{
			uint32 tile_idx_x = col / TILE_WIDTH;
			uint32 tile_idx = tile_idx_y * tiles_width_ + tile_idx_x;
			uint32 intile_idx_x = col % TILE_WIDTH;
			uint32 sub_tile_idx = (intile_idx_y / SUB_TILE_HEIGHT) * (TILE_WIDTH / SUB_TILE_WIDTH) + intile_idx_x / SUB_TILE_WIDTH;
			float rcp_depth = simd_f32(masked_hiz_buffer_[tile_idx].z_min[0])[sub_tile_idx];
			float tmp_depth = 1.0f / rcp_depth;
			depth_max = (rcp_depth == 0 | depth_max >= tmp_depth) ? depth_max : tmp_depth;
			/*tmp_depth = std::min<float>(std::max<float>(tmp_depth, 0), 1);*/
			imm_result_buffer[write_idx++] = rcp_depth == 0 ? std::numeric_limits<float>::max() : tmp_depth;
		}
	}
	write_idx = 0;
	for (uint32 i = 0; i < height_ * width_; i++)
	{
		float depth = imm_result_buffer[i];
		float depth01 = 1 - depth / depth_max;
		depth01 = std::min<float>(std::max<float>(depth01, 0), 1);
		result_buffer[write_idx++] = static_cast<char>(255.0f * depth01);
		result_buffer[write_idx++] = static_cast<char>(255.0f * depth01);
		result_buffer[write_idx++] = static_cast<char>(255.0f * depth01);
	}
	stbi_write_png(file_name.str().c_str(), width_, height_, 3, result_buffer.data(), 0);
}
void MSOCManager::CollectOccluderTriangles()
{
	if (!is_tri_list_initialized) { InitMSOCTriangleLists(); }
	auto V = ptr_renderer_->cam_.GetViewMatrix();
	auto P = ptr_renderer_->cam_.GetProjMatrix();
	glm::mat4x4 VP = P * V;
	uint32_t val_idx = 0;
	for (int i = 0; i < ptr_renderer_->render_elements_.size(); i++)
	{
		RenderElement& render_element = ptr_renderer_->render_elements_[i];
		if (render_element.IsNotOutOfViewFrustum())
		{
			glm::mat4x4 currentmvp{ VP * render_element.GetModelMatrix<false>() };
			if (render_element.occlusion_type_ != ERenderElementTtype::OccludeeOnly)
			{
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
				uint32_t num_lane = SIMD_LANES;
				uint32_t num_vertex = render_element.occluder_triangles_.size() * 3;
				for (uint32_t vert_begin_idx = 0; vert_begin_idx < render_element.occluder_triangles_.size() * 3; vert_begin_idx += num_lane)
				{
					uint32_t vert_end_idx = std::min(vert_begin_idx + 8, num_vertex);
					__m256 obj_pos[3];//0x 1y 2z
					__m256 clip_space_pos[4];//0x 1y 2z 3w
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
						clip_space_pos[dim] = _mm256_fmadd_ps(obj_pos[0], mvp_matrix[0 * 4 + dim], _mm256_fmadd_ps(obj_pos[1], mvp_matrix[1 * 4 + dim], _mm256_fmadd_ps(obj_pos[2], mvp_matrix[2 * 4 + dim], mvp_matrix[3 * 4 + dim])));
					}
					////简单的吧所有的三角形clamp到near plane之内，会有误差，但省去了多边形clip。
					//__m256 rcp_w = _mm256_div_ps(_mm256_set1_ps(1.0f),_mm256_max_ps(ndc_pos[3], _mm256_set1_ps(ptr_renderer_->cam_.GetNear())));
					//for (int dim = 0; dim < 3; dim++)//perspective division.
					//{
					//	ndc_pos[dim] = _mm256_mul_ps(ndc_pos[dim], rcp_w);
					//}

					//__m256 screen_space_x = _mm256_ceil_ps(_mm256_add_ps(_mm256_mul_ps(ndc_pos[0], mm_half_width_), mm_center_x_));
					//__m256 screen_space_y = _mm256_floor_ps(_mm256_add_ps(_mm256_mul_ps(ndc_pos[1], mm_half_height_), mm_center_y_));
					for (uint32_t i = vert_begin_idx; i < vert_end_idx; i++)//write back the result into the array.
					{
						raw_tri_list_.ptr_tri[val_idx++] = simd_f32(clip_space_pos[0])[i % 8];//x
						raw_tri_list_.ptr_tri[val_idx++] = simd_f32(clip_space_pos[1])[i % 8];//y
						raw_tri_list_.ptr_tri[val_idx++] = simd_f32(clip_space_pos[3])[i % 8];//w
					}
				}
			}
		}
	}
	raw_tri_list_.num_tri = val_idx / 9;
	CullBackTriangleAndClip();
}

void MSOCManager::CullBackTriangleAndClip()
{
	__m256 vert_x[3];
	__m256 vert_y[3];
	__m256 vert_z[3];
	final_tri_list_.num_tri = 0;
	for (uint32 begin_tri_idx = 0; begin_tri_idx < raw_tri_list_.num_tri; begin_tri_idx += SIMD_LANES)
	{
		uint32 end_tri_idx = std::min(static_cast<uint32>(begin_tri_idx + SIMD_LANES), raw_tri_list_.num_tri);
		for (uint32 tri_idx = begin_tri_idx; tri_idx < end_tri_idx; ++tri_idx)
		{
			for (uint32_t vert_idx = 0; vert_idx < 3; vert_idx++)
			{
				simd_f32(vert_x[vert_idx])[tri_idx % SIMD_LANES] = raw_tri_list_.ptr_tri[tri_idx * 9 + vert_idx * 3];
				simd_f32(vert_y[vert_idx])[tri_idx % SIMD_LANES] = raw_tri_list_.ptr_tri[tri_idx * 9 + vert_idx * 3 + 1];
				simd_f32(vert_z[vert_idx])[tri_idx % SIMD_LANES] = raw_tri_list_.ptr_tri[tri_idx * 9 + vert_idx * 3 + 2];
			}
		}
		// Perform backface test. 

		__m256 vert_x_after_div[3];
		__m256 vert_y_after_div[3];
		__m256 vert_rcp_w[3];
		for (uint32_t vert_idx = 0; vert_idx < 3; vert_idx++)//do projective divide first for more powerful backface cull
		{
			vert_rcp_w[vert_idx] = _mm256_div_ps(_mm256_set1_ps(1.0f), vert_z[vert_idx]);
			vert_x_after_div[vert_idx] = _mm256_mul_ps(vert_x[vert_idx], vert_rcp_w[vert_idx]);
			vert_y_after_div[vert_idx] = _mm256_mul_ps(vert_y[vert_idx], vert_rcp_w[vert_idx]);
		}
		__m256 triArea1 = _mm256_mul_ps(_mm256_sub_ps(vert_x_after_div[1], vert_x_after_div[0]), _mm256_sub_ps(vert_y_after_div[2], vert_y_after_div[0]));
		__m256 triArea2 = _mm256_mul_ps(_mm256_sub_ps(vert_y_after_div[1], vert_y_after_div[0]), _mm256_sub_ps(vert_x_after_div[2], vert_x_after_div[0]));
		__m256 triArea = _mm256_sub_ps(triArea1, triArea2);//cross(vec01, vec02) > 0 不论左右手坐标系，这两个向量叉乘为正，则是逆时针
		__m256 ccwMask = _mm256_cmpgt_ps(triArea, _mm256_setzero_ps());
		// Perform Clip:
		__m256 top_bound = _mm256_set1_ps(static_cast<float>(height_));
		__m256 bottom_bound = _mm256_set1_ps(0.f);
		__m256 left_bound = _mm256_set1_ps(0.f);
		__m256 right_bound = _mm256_set1_ps(static_cast<float>(width_));
		__m256 point_top_out_mask[3];
		__m256 point_bottom_out_mask[3];
		__m256 point_right_out_mask[3];
		__m256 point_left_out_mask[3];
		__m256 point_near_out_mask[3];
		for (uint32_t vert_idx = 0; vert_idx < 3; vert_idx++)
		{
			point_top_out_mask[vert_idx] = _mm256_cmpgt_ps(vert_y[vert_idx], vert_z[vert_idx]);
			point_bottom_out_mask[vert_idx] = _mm256_cmpgt_ps(_mm256_neg_ps(vert_z[vert_idx]), vert_y[vert_idx]);
			point_right_out_mask[vert_idx] = _mm256_cmpgt_ps(vert_x[vert_idx], vert_z[vert_idx]);
			point_left_out_mask[vert_idx] = _mm256_cmpgt_ps(_mm256_neg_ps(vert_z[vert_idx]), vert_x[vert_idx]);
			point_near_out_mask[vert_idx] = _mm256_cmpge_ps(_mm256_set1_ps(0.f), vert_z[vert_idx]);
		}
		__m256 tri_outside_of_plane[5];//near up down right left.

		__m256 tri_inside_of_plane[5];//near up down right left.
		tri_outside_of_plane[0] = _mm256_and_ps(point_near_out_mask[0], _mm256_and_ps(point_near_out_mask[1], point_near_out_mask[2]));
		tri_outside_of_plane[1] = _mm256_and_ps(point_top_out_mask[0], _mm256_and_ps(point_top_out_mask[1], point_top_out_mask[2]));
		tri_outside_of_plane[2] = _mm256_and_ps(point_bottom_out_mask[0], _mm256_and_ps(point_bottom_out_mask[1], point_bottom_out_mask[2]));
		tri_outside_of_plane[3] = _mm256_and_ps(point_right_out_mask[0], _mm256_and_ps(point_right_out_mask[1], point_right_out_mask[2]));
		tri_outside_of_plane[4] = _mm256_and_ps(point_left_out_mask[0], _mm256_and_ps(point_left_out_mask[1], point_left_out_mask[2]));
		__m256 tri_outside_mask = tri_outside_of_plane[0];
		for (uint32 plane_idx = 1; plane_idx < 5; plane_idx++)
		{
			tri_outside_mask = _mm256_or_ps(tri_outside_mask, tri_outside_of_plane[plane_idx]);
		}
		tri_inside_of_plane[0] = _mm256_and_ps(_mm256_neg_ps(point_near_out_mask[0]), _mm256_and_ps(_mm256_neg_ps(point_near_out_mask[1]), _mm256_neg_ps(point_near_out_mask[2])));
		tri_inside_of_plane[1] = _mm256_and_ps(_mm256_neg_ps(point_top_out_mask[0]), _mm256_and_ps(_mm256_neg_ps(point_top_out_mask[1]), _mm256_neg_ps(point_top_out_mask[2])));
		tri_inside_of_plane[2] = _mm256_and_ps(_mm256_neg_ps(point_bottom_out_mask[0]), _mm256_and_ps(_mm256_neg_ps(point_bottom_out_mask[1]), _mm256_neg_ps(point_bottom_out_mask[2])));
		tri_inside_of_plane[3] = _mm256_and_ps(_mm256_neg_ps(point_right_out_mask[0]), _mm256_and_ps(_mm256_neg_ps(point_right_out_mask[1]), _mm256_neg_ps(point_right_out_mask[2])));
		tri_inside_of_plane[4] = _mm256_and_ps(_mm256_neg_ps(point_left_out_mask[0]), _mm256_and_ps(_mm256_neg_ps(point_left_out_mask[1]), _mm256_neg_ps(point_left_out_mask[2])));
		__m256 tri_inside_mask = tri_inside_of_plane[0];
		for (uint32 plane_idx = 1; plane_idx < 5; plane_idx++)
		{
			tri_inside_mask = _mm256_and_ps(tri_inside_mask, tri_inside_of_plane[plane_idx]);
		}
		uint32_t tri_mask = ~((~0) << (end_tri_idx - begin_tri_idx));
		uint32_t front_face_mask = _mm256_movemask_ps(ccwMask);
		uint32_t intersect_mask = tri_mask & (front_face_mask | (~_mm256_movemask_ps(tri_inside_of_plane[0]))) & _mm256_movemask_ps(_mm256_and_ps(_mm256_neg_ps(tri_inside_mask), _mm256_neg_ps(tri_outside_mask)));
		uint32_t front_inside_mask = tri_mask & front_face_mask & _mm256_movemask_ps(tri_inside_mask);
		uint32 tri_intersect_plane[5];
		for (uint32 plane_idx = 0; plane_idx < 5; plane_idx++)
		{
			tri_intersect_plane[plane_idx] = (~_mm256_movemask_ps(tri_inside_of_plane[plane_idx])) & (~_mm256_movemask_ps(tri_outside_of_plane[plane_idx]));
		}
		for (uint32 tri_idx = begin_tri_idx; tri_idx < end_tri_idx; ++tri_idx)
		{
			uint32 tri_idx_inner = tri_idx % SIMD_LANES;
			if ((front_inside_mask & (1 << (tri_idx % SIMD_LANES))) != 0)
			{
				for (uint32_t vert_idx = 0; vert_idx < 3; vert_idx++)
				{
					uint32_t begin_write_idx = final_tri_list_.num_tri * 9 + vert_idx * 3;
					uint32_t begin_read_idx = tri_idx * 9 + vert_idx * 3;
					final_tri_list_.ptr_tri[begin_write_idx] = simd_f32(vert_x_after_div[vert_idx])[tri_idx_inner] * half_width_ + half_width_;
					final_tri_list_.ptr_tri[begin_write_idx + 1] = simd_f32(vert_y_after_div[vert_idx])[tri_idx_inner] * half_height_ + half_height_;
					final_tri_list_.ptr_tri[begin_write_idx + 2] = simd_f32(vert_rcp_w[vert_idx])[tri_idx_inner];
				}
				final_tri_list_.num_tri++;
			}
		}

		//add clipped triangles:

		__m128 vtxBuf[2][8];

		while (intersect_mask)
		{
			// Find and setup next triangle to clip
			unsigned int triIdx = find_clear_lsb(&intersect_mask);
			unsigned int triBit = (1U << triIdx);
			assert(triIdx < SIMD_LANES);

			int bufIdx = 0;
			int nClippedVerts = 3;
			for (int i = 0; i < 3; i++)
				vtxBuf[0][i] = _mm_setr_ps(simd_f32(vert_x[i])[triIdx], simd_f32(vert_y[i])[triIdx], simd_f32(vert_z[i])[triIdx], 1.0f);

			// Clip triangle with straddling planes. 
			for (int i = 0; i < 5; ++i)
			{
				if (tri_intersect_plane[i] & triBit) // <- second part maybe not needed?
				{
					nClippedVerts = ClipPolygon(vtxBuf[bufIdx ^ 1], vtxBuf[bufIdx], mm_frustum_planes_[i], nClippedVerts);
					bufIdx ^= 1;
				}
			}

			if (nClippedVerts >= 3)
			{
				// Write all triangles into the clip buffer and process them next loop iteration
				uint32 write_val_idx = final_tri_list_.num_tri * 9;
				for (uint32 vert_idx = 0; vert_idx < 3; vert_idx++)
				{
					for (uint32 dim = 0; dim < 3; dim++)
					{
						final_tri_list_.ptr_tri[write_val_idx++] = simd_f32(vtxBuf[bufIdx][vert_idx])[dim];
					}
					float tmp_rcp_w = 1.f / final_tri_list_.ptr_tri[write_val_idx - 1];
					final_tri_list_.ptr_tri[write_val_idx - 3] = final_tri_list_.ptr_tri[write_val_idx - 3] * tmp_rcp_w * half_width_ + half_width_;
					final_tri_list_.ptr_tri[write_val_idx - 2] = final_tri_list_.ptr_tri[write_val_idx - 2] * tmp_rcp_w * half_height_ + half_height_;
					final_tri_list_.ptr_tri[write_val_idx - 1] = tmp_rcp_w;
				}
				for (int i = 2; i < nClippedVerts - 1; i++)
				{
					for (uint32 vert_idx = 0; vert_idx < 3; vert_idx++)
					{
						for (uint32 dim = 0; dim < 3; dim++)
						{
							final_tri_list_.ptr_tri[write_val_idx++] = simd_f32(vtxBuf[bufIdx][0 == vert_idx ? 0 : i + vert_idx - 1])[dim];
						}
						float tmp_rcp_w = 1.f / final_tri_list_.ptr_tri[write_val_idx - 1];
						final_tri_list_.ptr_tri[write_val_idx - 3] = final_tri_list_.ptr_tri[write_val_idx - 3] * tmp_rcp_w * half_width_ + half_width_;
						final_tri_list_.ptr_tri[write_val_idx - 2] = final_tri_list_.ptr_tri[write_val_idx - 2] * tmp_rcp_w * half_height_ + half_height_;
						final_tri_list_.ptr_tri[write_val_idx - 1] = tmp_rcp_w;
					}
				}
				final_tri_list_.num_tri += nClippedVerts - 2;
			}
		}
	}
}
