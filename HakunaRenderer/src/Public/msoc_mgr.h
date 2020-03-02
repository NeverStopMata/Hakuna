#pragma once
#include <glm/glm.hpp> 
#include <array>
#include <vector>
#include <memory>
#include "bbox.h"

#define FORCE_INLINE __forceinline


#define TILE_WIDTH_SHIFT        5
#define TILE_HEIGHT_SHIFT	    3
#define TILE_WIDTH             (1 << TILE_WIDTH_SHIFT)
#define TILE_HEIGHT            (1 << TILE_HEIGHT_SHIFT)

#define SIMD_LANES 8
#define SIMD_ALL_LANES_MASK     ((1 << SIMD_LANES) - 1)
#define SIMD_BITS_ZERO          _mm256_setzero_si256()
#define SIMD_LANE_IDX			_mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7)
#define SIMD_BITS_ONE			_mm256_set1_epi32(~0)
#define SIMD_SHUFFLE_SCANLINE_TO_SUBTILES _mm256_setr_epi8(0x0, 0x4, 0x8, 0xC, 0x1, 0x5, 0x9, 0xD, 0x2, 0x6, 0xA, 0xE, 0x3, 0x7, 0xB, 0xF, 0x0, 0x4, 0x8, 0xC, 0x1, 0x5, 0x9, 0xD, 0x2, 0x6, 0xA, 0xE, 0x3, 0x7, 0xB, 0xF)
#define SIMD_TILE_WIDTH			_mm256_set1_epi32(TILE_WIDTH)

#define _mm256_cmpge_ps(a,b)          _mm256_cmp_ps(a, b, _CMP_GE_OQ)
#define _mm256_cmpgt_ps(a,b)          _mm256_cmp_ps(a, b, _CMP_GT_OQ)
#define _mm256_cmpeq_ps(a,b)          _mm256_cmp_ps(a, b, _CMP_EQ_OQ)
#define _mm256_neg_ps(a)              _mm256_xor_ps((a), _mm256_set1_ps(-0.0f))
#define _mm256_not_epi32(a)           _mm256_xor_si256((a), _mm256_set1_epi32(~0))
#define FP_BITS             (19 - TILE_HEIGHT_SHIFT)
// Sub-tiles (used for updating the masked HiZ buffer) are 8x4 tiles, so there are 4x2 sub-tiles in a tile
#define SUB_TILE_WIDTH          8
#define SUB_TILE_HEIGHT         4
#define GUARD_BAND_PIXEL_SIZE   1.0f
#define BIG_TRIANGLE            3
#define SIMD_SUB_TILE_COL_OFFSET_F _mm256_setr_ps(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3, 0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3)
#define SIMD_SUB_TILE_ROW_OFFSET_F _mm256_setr_ps(0, 0, 0, 0, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT)
#define QUICK_MASK              1
FORCE_INLINE unsigned long find_clear_lsb(unsigned int* mask)
{
	unsigned long idx;
	_BitScanForward(&idx, *mask);
	*mask &= *mask - 1;
	return idx;
}

template<typename T, typename Y> FORCE_INLINE T simd_cast(Y A);
template<> FORCE_INLINE __m128  simd_cast<__m128>(float A) { return _mm_set1_ps(A); }
template<> FORCE_INLINE __m128  simd_cast<__m128>(__m128i A) { return _mm_castsi128_ps(A); }
template<> FORCE_INLINE __m128  simd_cast<__m128>(__m128 A) { return A; }
template<> FORCE_INLINE __m128i simd_cast<__m128i>(int A) { return _mm_set1_epi32(A); }
template<> FORCE_INLINE __m128i simd_cast<__m128i>(__m128 A) { return _mm_castps_si128(A); }
template<> FORCE_INLINE __m128i simd_cast<__m128i>(__m128i A) { return A; }
template<> FORCE_INLINE __m256  simd_cast<__m256>(float A) { return _mm256_set1_ps(A); }
template<> FORCE_INLINE __m256  simd_cast<__m256>(__m256i A) { return _mm256_castsi256_ps(A); }
template<> FORCE_INLINE __m256  simd_cast<__m256>(__m256 A) { return A; }
template<> FORCE_INLINE __m256i simd_cast<__m256i>(int A) { return _mm256_set1_epi32(A); }
template<> FORCE_INLINE __m256i simd_cast<__m256i>(__m256 A) { return _mm256_castps_si256(A); }
template<> FORCE_INLINE __m256i simd_cast<__m256i>(__m256i A) { return A; }

#define MAKE_ACCESSOR(name, simd_type, base_type, is_const, elements) \
	FORCE_INLINE is_const base_type * name(is_const simd_type &a) { \
		union accessor { simd_type m_native; base_type m_array[elements]; }; \
		is_const accessor *acs = reinterpret_cast<is_const accessor*>(&a); \
		return acs->m_array; \
	}

MAKE_ACCESSOR(simd_f32, __m128, float, , 4)
MAKE_ACCESSOR(simd_f32, __m128, float, const, 4)
MAKE_ACCESSOR(simd_i32, __m128i, int, , 4)
MAKE_ACCESSOR(simd_i32, __m128i, int, const, 4)

MAKE_ACCESSOR(simd_f32, __m256, float, , 8)
MAKE_ACCESSOR(simd_f32, __m256, float, const, 8)
MAKE_ACCESSOR(simd_i32, __m256i, int, , 8)
MAKE_ACCESSOR(simd_i32, __m256i, int, const, 8)


enum ECullingResult
{
	VISIBLE = 0x0,
	OCCLUDED = 0x1,
	VIEW_CULLED = 0x3
};

class RenderElement;
class HakunaRenderer;
struct Triangle
{
	std::array<glm::vec3, 3> positions;
};

struct ZTile
{
	__m256        mZMin[2];
	__m256i       mMask;
};

struct ScissorRect
{
	ScissorRect() {}
	ScissorRect(int minX, int minY, int maxX, int maxY) :
		mMinX(minX), mMinY(minY), mMaxX(maxX), mMaxY(maxY) {}

	int mMinX; //!< Screen space X coordinate for left side of scissor rect, inclusive and must be a multiple of 32
	int mMinY; //!< Screen space Y coordinate for bottom side of scissor rect, inclusive and must be a multiple of 8
	int mMaxX; //!< Screen space X coordinate for right side of scissor rect, <B>non</B> inclusive and must be a multiple of 32
	int mMaxY; //!< Screen space Y coordinate for top side of scissor rect, <B>non</B> inclusive and must be a multiple of 8
};

struct TriList
{
	float* ptr_tri = nullptr;
	uint32_t num_tri = 0;
};
class MSOCManager
{
public:
	static MSOCManager* GetInstance()
	{
		if (p_instance_ == nullptr)
			p_instance_ = new MSOCManager();
		return p_instance_;
	}
	void InitMSOCManager(HakunaRenderer* render_ptr);

	void CollectOccluderTriangles();
	//beigin:                            *******************************************************/

	FORCE_INLINE void ComputeBoundingBox(__m256i& bbminX, __m256i& bbminY, __m256i& bbmaxX, __m256i& bbmaxY, const __m256* vX, const __m256* vY, const ScissorRect* scissor)
	{
		static const __m256i SIMD_PAD_W_MASK = _mm256_set1_epi32(~(TILE_WIDTH - 1));
		static const __m256i SIMD_PAD_H_MASK = _mm256_set1_epi32(~(TILE_HEIGHT - 1));

		// Find std::min/Max vertices
		bbminX = _mm256_cvttps_epi32(_mm256_min_ps(vX[0], _mm256_min_ps(vX[1], vX[2])));
		bbminY = _mm256_cvttps_epi32(_mm256_min_ps(vY[0], _mm256_min_ps(vY[1], vY[2])));
		bbmaxX = _mm256_cvttps_epi32(_mm256_max_ps(vX[0], _mm256_max_ps(vX[1], vX[2])));
		bbmaxY = _mm256_cvttps_epi32(_mm256_max_ps(vY[0], _mm256_max_ps(vY[1], vY[2])));

		// Clamp to tile boundaries
		bbminX = _mm256_and_si256(bbminX, SIMD_PAD_W_MASK);
		bbmaxX = _mm256_and_si256(_mm256_add_epi32(bbmaxX, _mm256_set1_epi32(TILE_WIDTH)), SIMD_PAD_W_MASK);
		bbminY = _mm256_and_si256(bbminY, SIMD_PAD_H_MASK);
		bbmaxY = _mm256_and_si256(_mm256_add_epi32(bbmaxY, _mm256_set1_epi32(TILE_HEIGHT)), SIMD_PAD_H_MASK);

		// Clip to scissor
		bbminX = _mm256_max_epi32(bbminX, _mm256_set1_epi32(scissor->mMinX));
		bbmaxX = _mm256_min_epi32(bbmaxX, _mm256_set1_epi32(scissor->mMaxX));
		bbminY = _mm256_max_epi32(bbminY, _mm256_set1_epi32(scissor->mMinY));
		bbmaxY = _mm256_min_epi32(bbmaxY, _mm256_set1_epi32(scissor->mMaxY));
	}

	FORCE_INLINE void ComputeDepthPlane(const __m256* pVtxX, const __m256* pVtxY, const __m256* pVtxZ, __m256& zPixelDx, __m256& zPixelDy) const
	{
		// Setup z(x,y) = z0 + dx*x + dy*y screen space depth plane equation
		__m256 x2 = _mm256_sub_ps(pVtxX[2], pVtxX[0]);
		__m256 x1 = _mm256_sub_ps(pVtxX[1], pVtxX[0]);
		__m256 y1 = _mm256_sub_ps(pVtxY[1], pVtxY[0]);
		__m256 y2 = _mm256_sub_ps(pVtxY[2], pVtxY[0]);
		__m256 z1 = _mm256_sub_ps(pVtxZ[1], pVtxZ[0]);
		__m256 z2 = _mm256_sub_ps(pVtxZ[2], pVtxZ[0]);
		__m256 d = _mm256_div_ps(_mm256_set1_ps(1.0f), _mm256_fmsub_ps(x1, y2, _mm256_mul_ps(y1, x2)));
		zPixelDx = _mm256_mul_ps(_mm256_fmsub_ps(z1, y2, _mm256_mul_ps(y1, z2)), d);
		zPixelDy = _mm256_mul_ps(_mm256_fmsub_ps(x1, z2, _mm256_mul_ps(z1, x2)), d);
	}

	FORCE_INLINE void SortVertices(__m256* vX, __m256* vY)
	{
		// Rotate the triangle in the winding order until v0 is the vertex with lowest Y value
		for (int i = 0; i < 2; i++)
		{
			__m256 ey1 = _mm256_sub_ps(vY[1], vY[0]);
			__m256 ey2 = _mm256_sub_ps(vY[2], vY[0]);
			//这个ps packed single 作为blend的factor时，貌似.... 只看第一位，也就是符号位，所以若ey1或ey2为负数，就需要交换。
			__m256 swapMask = _mm256_or_ps(_mm256_or_ps(ey1, ey2), simd_cast<__m256>(_mm256_cmpeq_epi32(simd_cast<__m256i>(ey2), SIMD_BITS_ZERO)));
			__m256 sX, sY;
			__m256i test = simd_cast<__m256i>(swapMask);
			sX = _mm256_blendv_ps(vX[2], vX[0], swapMask);
			vX[0] = _mm256_blendv_ps(vX[0], vX[1], swapMask);
			vX[1] = _mm256_blendv_ps(vX[1], vX[2], swapMask);
			vX[2] = sX;
			sY = _mm256_blendv_ps(vY[2], vY[0], swapMask);
			vY[0] = _mm256_blendv_ps(vY[0], vY[1], swapMask);
			vY[1] = _mm256_blendv_ps(vY[1], vY[2], swapMask);
			vY[2] = sY;
		}
	}

	FORCE_INLINE void UpdateTileQuick(int tileIdx, const __m256i& coverage, const __m256& zTriv)
	{
		// Update heuristic used in the paper "Masked Software Occlusion Culling", 
		// good balance between performance and accuracy
		assert(tileIdx >= 0 && tileIdx < tiles_width_ * tiles_height_);

		__m256i mask = masked_hiz_buffer_[tileIdx].mMask;
		__m256* zMin = masked_hiz_buffer_[tileIdx].mZMin;

		// Swizzle coverage mask to 8x4 subtiles and test if any subtiles are not covered at all
		__m256i rastMask = _mm256_shuffle_epi8(coverage, SIMD_SHUFFLE_SCANLINE_TO_SUBTILES);// _mmw_transpose_epi8(coverage);
		__m256i deadLane = _mm256_cmpeq_epi32(rastMask, SIMD_BITS_ZERO);

		// Mask out all subtiles failing the depth test (don't update these subtiles)
		deadLane = _mm256_or_si256(deadLane, _mm256_srai_epi32(simd_cast<__m256i>(_mm256_sub_ps(zTriv, zMin[0])), 31));
		rastMask = _mm256_andnot_si256(deadLane, rastMask);

		// Use distance heuristic to discard layer 1 if incoming triangle is significantly nearer to observer
		// than the buffer contents. See Section 3.2 in "Masked Software Occlusion Culling"
		__m256i coveredLane = _mm256_cmpeq_epi32(rastMask, SIMD_BITS_ONE);
		__m256 diff = _mm256_fmsub_ps(zMin[1], _mm256_set1_ps(2.0f), _mm256_add_ps(zTriv, zMin[0]));
		__m256i discardLayerMask = _mm256_andnot_si256(deadLane, _mm256_or_si256(_mm256_srai_epi32(simd_cast<__m256i>(diff), 31), coveredLane));

		// Update the mask with incoming triangle coverage
		mask = _mm256_or_si256(_mm256_andnot_si256(discardLayerMask, mask), rastMask);

		__m256i maskFull = _mm256_cmpeq_epi32(mask, SIMD_BITS_ONE);

		// Compute new value for zMin[1]. This has one of four outcomes: zMin[1] = min(zMin[1], zTriv),  zMin[1] = zTriv, 
		// zMin[1] = FLT_MAX or unchanged, depending on if the layer is updated, discarded, fully covered, or not updated
		__m256 opA = _mm256_blendv_ps(zTriv, zMin[1], simd_cast<__m256>(deadLane));
		__m256 opB = _mm256_blendv_ps(zMin[1], zTriv, simd_cast<__m256>(discardLayerMask));
		__m256 z1min = _mm256_min_ps(opA, opB);
		zMin[1] = _mm256_blendv_ps(z1min, _mm256_set1_ps(FLT_MAX), simd_cast<__m256>(maskFull));

		// Propagate zMin[1] back to zMin[0] if tile was fully covered, and update the mask
		zMin[0] = _mm256_blendv_ps(zMin[0], z1min, simd_cast<__m256>(maskFull));
		masked_hiz_buffer_[tileIdx].mMask = _mm256_andnot_si256(maskFull, mask);
	}

	template<int TEST_Z, int NRIGHT, int NLEFT>
	FORCE_INLINE int TraverseScanline(int leftOffset, int rightOffset, int tileIdx, int rightEvent, int leftEvent, const __m256i* events, const __m256& zTriMin, const __m256& zTriMax, const __m256& iz0, float zx)
	{
		// Floor edge events to integer pixel coordinates (shift out fixed point bits)
		int eventOffset = leftOffset << TILE_WIDTH_SHIFT;
		__m256i right[NRIGHT], left[NLEFT];
		for (int i = 0; i < NRIGHT; ++i)
			right[i] = _mm256_max_epi32(_mm256_sub_epi32(_mm256_srai_epi32(events[rightEvent + i], FP_BITS), _mm256_set1_epi32(eventOffset)), SIMD_BITS_ZERO);
		for (int i = 0; i < NLEFT; ++i)
			left[i] = _mm256_max_epi32(_mm256_sub_epi32(_mm256_srai_epi32(events[leftEvent - i], FP_BITS), _mm256_set1_epi32(eventOffset)), SIMD_BITS_ZERO);

		__m256 z0 = _mm256_add_ps(iz0, _mm256_set1_ps(zx * leftOffset));
		int tileIdxEnd = tileIdx + rightOffset;
		tileIdx += leftOffset;
		for (;;)
		{
			// Only use the reference layer (layer 0) to cull as it is always conservative
			__m256 zMinBuf = masked_hiz_buffer_[tileIdx].mZMin[0];

			__m256 dist0 = _mm256_sub_ps(zTriMax, zMinBuf);
			if (_mm256_movemask_ps(dist0) != SIMD_ALL_LANES_MASK)//三角形最近的点的确比depth buffer中记录的参考值来得近
			{
				// Compute coverage mask for entire 32xN using shift operations
				__m256i accumulatedMask = _mm256_sllv_epi32(SIMD_BITS_ONE, left[0]);
				for (int i = 1; i < NLEFT; ++i)
					accumulatedMask = _mm256_and_si256(accumulatedMask, _mm256_sllv_epi32(SIMD_BITS_ONE, left[i]));
				for (int i = 0; i < NRIGHT; ++i)
					accumulatedMask = _mm256_andnot_si256(_mm256_sllv_epi32(SIMD_BITS_ONE, right[i]), accumulatedMask);

				if (TEST_Z)
				{
					// Perform a conservative visibility test (test zMax against buffer for each covered 8x4 subtile)
					__m256 zSubTileMax = _mm256_min_ps(z0, zTriMax);
					__m256i zPass = simd_cast<__m256i>(_mm256_cmpge_ps(zSubTileMax, zMinBuf));

					__m256i rastMask = _mm256_shuffle_epi8(accumulatedMask, SIMD_SHUFFLE_SCANLINE_TO_SUBTILES);
					__m256i deadLane = _mm256_cmpeq_epi32(rastMask, SIMD_BITS_ZERO);
					zPass = _mm256_andnot_si256(deadLane, zPass);

					if (!_mm256_testz_si256(zPass, zPass))
						return ECullingResult::VISIBLE;
				}
				else
				{
					// Compute interpolated min for each 8x4 subtile and update the masked hierarchical z buffer entry
					__m256 zSubTileMin = _mm256_max_ps(z0, zTriMin);
#if QUICK_MASK != 0
					UpdateTileQuick(tileIdx, accumulatedMask, zSubTileMin);
#else 
					UpdateTileAccurate(tileIdx, accumulatedMask, zSubTileMin);
#endif
				}
			}

			// Update buffer address, interpolate z and edge events
			tileIdx++;
			if (tileIdx >= tileIdxEnd)
				break;
			z0 = _mm256_add_ps(z0, _mm256_set1_ps(zx));
			for (int i = 0; i < NRIGHT; ++i)
				right[i] = _mm256_subs_epu16(right[i], SIMD_TILE_WIDTH);	// Trick, use sub saturated to avoid checking against < 0 for shift (values should fit in 16 bits)
			for (int i = 0; i < NLEFT; ++i)
				left[i] = _mm256_subs_epu16(left[i], SIMD_TILE_WIDTH);
		}

		return TEST_Z ? ECullingResult::OCCLUDED : ECullingResult::VISIBLE;
	}

	template<int TEST_Z, int TIGHT_TRAVERSAL, int MID_VTX_RIGHT>
	FORCE_INLINE int RasterizeTriangle(unsigned int triIdx, int bbWidth, int tileRowIdx, int tileMidRowIdx, int tileEndRowIdx, const __m256i* eventStart, const __m256i* slope, const __m256i* slopeTileDelta, const __m256& zTriMin, const __m256& zTriMax, __m256& z0, float zx, float zy)
	{

		int cullResult;
#define LEFT_EDGE_BIAS 0
#define RIGHT_EDGE_BIAS 0
#define UPDATE_TILE_EVENTS_Y(i)		triEvent[i] = _mm256_add_epi32(triEvent[i], triSlopeTileDelta[i]);

		// Get deltas used to increment edge events each time we traverse one scanline of tiles
		__m256i triSlopeTileDelta[3];
		triSlopeTileDelta[0] = _mm256_set1_epi32(simd_i32(slopeTileDelta[0])[triIdx]);
		triSlopeTileDelta[1] = _mm256_set1_epi32(simd_i32(slopeTileDelta[1])[triIdx]);
		triSlopeTileDelta[2] = _mm256_set1_epi32(simd_i32(slopeTileDelta[2])[triIdx]);

		// Setup edge events for first batch of SIMD_LANES scanlines
		__m256i triEvent[3];
		triEvent[0] = _mm256_add_epi32(_mm256_set1_epi32(simd_i32(eventStart[0])[triIdx]), _mm256_mullo_epi32(SIMD_LANE_IDX, _mm256_set1_epi32(simd_i32(slope[0])[triIdx])));
		triEvent[1] = _mm256_add_epi32(_mm256_set1_epi32(simd_i32(eventStart[1])[triIdx]), _mm256_mullo_epi32(SIMD_LANE_IDX, _mm256_set1_epi32(simd_i32(slope[1])[triIdx])));
		triEvent[2] = _mm256_add_epi32(_mm256_set1_epi32(simd_i32(eventStart[2])[triIdx]), _mm256_mullo_epi32(SIMD_LANE_IDX, _mm256_set1_epi32(simd_i32(slope[2])[triIdx])));

		// For big triangles track start & end tile for each scanline and only traverse the valid region
		int startDelta, endDelta, topDelta, startEvent, endEvent, topEvent;
		if (TIGHT_TRAVERSAL)
		{
			startDelta = simd_i32(slopeTileDelta[2])[triIdx] + LEFT_EDGE_BIAS;
			endDelta = simd_i32(slopeTileDelta[0])[triIdx] + RIGHT_EDGE_BIAS;
			topDelta = simd_i32(slopeTileDelta[1])[triIdx] + (MID_VTX_RIGHT ? RIGHT_EDGE_BIAS : LEFT_EDGE_BIAS);

			// Compute conservative bounds for the edge events over a 32xN tile
			startEvent = simd_i32(eventStart[2])[triIdx] + std::min(0, startDelta);
			endEvent = simd_i32(eventStart[0])[triIdx] + std::max(0, endDelta) + (TILE_WIDTH << FP_BITS);
			if (MID_VTX_RIGHT)
				topEvent = simd_i32(eventStart[1])[triIdx] + std::max(0, topDelta) + (TILE_WIDTH << FP_BITS);
			else
				topEvent = simd_i32(eventStart[1])[triIdx] + std::min(0, topDelta);
		}

		if (tileRowIdx <= tileMidRowIdx)
		{
			int tileStopIdx = std::min(tileEndRowIdx, tileMidRowIdx);
			// Traverse the bottom half of the triangle
			while (tileRowIdx < tileStopIdx)
			{
				int start = 0, end = bbWidth;
				if (TIGHT_TRAVERSAL)
				{
					// Compute tighter start and endpoints to avoid traversing empty space
					start = std::max(0, std::min(bbWidth - 1, startEvent >> (TILE_WIDTH_SHIFT + FP_BITS)));
					end = std::min(bbWidth, ((int)endEvent >> (TILE_WIDTH_SHIFT + FP_BITS)));
					startEvent += startDelta;
					endEvent += endDelta;
				}

				// Traverse the scanline and update the masked hierarchical z buffer
				cullResult = TraverseScanline<TEST_Z, 1, 1>(start, end, tileRowIdx, 0, 2, triEvent, zTriMin, zTriMax, z0, zx);

				if (TEST_Z && cullResult == ECullingResult::VISIBLE) // Early out if performing occlusion query
					return ECullingResult::VISIBLE;

				// move to the next scanline of tiles, update edge events and interpolate z
				tileRowIdx += tiles_width_;
				z0 = _mm256_add_ps(z0, _mm256_set1_ps(zy));
				UPDATE_TILE_EVENTS_Y(0);
				UPDATE_TILE_EVENTS_Y(2);
			}

			// Traverse the middle scanline of tiles. We must consider all three edges only in this region
			if (tileRowIdx < tileEndRowIdx)
			{
				int start = 0, end = bbWidth;
				if (TIGHT_TRAVERSAL)
				{
					// Compute tighter start and endpoints to avoid traversing lots of empty space
					start = std::max(0, std::min(bbWidth - 1, startEvent >> (TILE_WIDTH_SHIFT + FP_BITS)));
					end = std::min(bbWidth, ((int)endEvent >> (TILE_WIDTH_SHIFT + FP_BITS)));

					// Switch the traversal start / end to account for the upper side edge
					endEvent = MID_VTX_RIGHT ? topEvent : endEvent;
					endDelta = MID_VTX_RIGHT ? topDelta : endDelta;
					startEvent = MID_VTX_RIGHT ? startEvent : topEvent;
					startDelta = MID_VTX_RIGHT ? startDelta : topDelta;
					startEvent += startDelta;
					endEvent += endDelta;
				}

				// Traverse the scanline and update the masked hierarchical z buffer. 
				if (MID_VTX_RIGHT)
					cullResult = TraverseScanline<TEST_Z, 2, 1>(start, end, tileRowIdx, 0, 2, triEvent, zTriMin, zTriMax, z0, zx);
				else
					cullResult = TraverseScanline<TEST_Z, 1, 2>(start, end, tileRowIdx, 0, 2, triEvent, zTriMin, zTriMax, z0, zx);

				if (TEST_Z && cullResult == ECullingResult::VISIBLE) // Early out if performing occlusion query
					return ECullingResult::VISIBLE;

				tileRowIdx += tiles_width_;
			}

			// Traverse the top half of the triangle
			if (tileRowIdx < tileEndRowIdx)
			{
				// move to the next scanline of tiles, update edge events and interpolate z
				z0 = _mm256_add_ps(z0, _mm256_set1_ps(zy));
				int i0 = MID_VTX_RIGHT + 0;
				int i1 = MID_VTX_RIGHT + 1;
				UPDATE_TILE_EVENTS_Y(i0);
				UPDATE_TILE_EVENTS_Y(i1);
				for (;;)
				{
					int start = 0, end = bbWidth;
					if (TIGHT_TRAVERSAL)
					{
						// Compute tighter start and endpoints to avoid traversing lots of empty space
						start = std::max(0, std::min(bbWidth - 1, startEvent >> (TILE_WIDTH_SHIFT + FP_BITS)));
						end = std::min(bbWidth, ((int)endEvent >> (TILE_WIDTH_SHIFT + FP_BITS)));
						startEvent += startDelta;
						endEvent += endDelta;
					}

					// Traverse the scanline and update the masked hierarchical z buffer
					cullResult = TraverseScanline<TEST_Z, 1, 1>(start, end, tileRowIdx, MID_VTX_RIGHT + 0, MID_VTX_RIGHT + 1, triEvent, zTriMin, zTriMax, z0, zx);

					if (TEST_Z && cullResult == ECullingResult::VISIBLE) // Early out if performing occlusion query
						return ECullingResult::VISIBLE;

					// move to the next scanline of tiles, update edge events and interpolate z
					tileRowIdx += tiles_width_;
					if (tileRowIdx >= tileEndRowIdx)
						break;
					z0 = _mm256_add_ps(z0, _mm256_set1_ps(zy));
					UPDATE_TILE_EVENTS_Y(i0);
					UPDATE_TILE_EVENTS_Y(i1);
				}
			}
		}
		else
		{
			if (TIGHT_TRAVERSAL)
			{
				// For large triangles, switch the traversal start / end to account for the upper side edge
				endEvent = MID_VTX_RIGHT ? topEvent : endEvent;
				endDelta = MID_VTX_RIGHT ? topDelta : endDelta;
				startEvent = MID_VTX_RIGHT ? startEvent : topEvent;
				startDelta = MID_VTX_RIGHT ? startDelta : topDelta;
			}

			// Traverse the top half of the triangle
			if (tileRowIdx < tileEndRowIdx)
			{
				int i0 = MID_VTX_RIGHT + 0;
				int i1 = MID_VTX_RIGHT + 1;
				for (;;)
				{
					int start = 0, end = bbWidth;
					if (TIGHT_TRAVERSAL)
					{
						// Compute tighter start and endpoints to avoid traversing lots of empty space
						start = std::max(0, std::min(bbWidth - 1, startEvent >> (TILE_WIDTH_SHIFT + FP_BITS)));
						end = std::min(bbWidth, ((int)endEvent >> (TILE_WIDTH_SHIFT + FP_BITS)));
						startEvent += startDelta;
						endEvent += endDelta;
					}

					// Traverse the scanline and update the masked hierarchical z buffer
					cullResult = TraverseScanline<TEST_Z, 1, 1>(start, end, tileRowIdx, MID_VTX_RIGHT + 0, MID_VTX_RIGHT + 1, triEvent, zTriMin, zTriMax, z0, zx);

					if (TEST_Z && cullResult == ECullingResult::VISIBLE) // Early out if performing occlusion query
						return ECullingResult::VISIBLE;

					// move to the next scanline of tiles, update edge events and interpolate z
					tileRowIdx += tiles_width_;
					if (tileRowIdx >= tileEndRowIdx)
						break;
					z0 = _mm256_add_ps(z0, _mm256_set1_ps(zy));
					UPDATE_TILE_EVENTS_Y(i0);
					UPDATE_TILE_EVENTS_Y(i1);
				}
			}
		}

		return TEST_Z ? ECullingResult::OCCLUDED : ECullingResult::VISIBLE;
	}

	template<bool TEST_Z>
	FORCE_INLINE int RasterizeTriangleBatch(__m256 pVtxX[3], __m256 pVtxY[3], __m256 pVtxZ[3], unsigned int triMask, const ScissorRect* scissor)
	{
		int cullResult = ECullingResult::VIEW_CULLED;

		//////////////////////////////////////////////////////////////////////////////
		// Compute bounding box and clamp to tile coordinates
		//////////////////////////////////////////////////////////////////////////////

		__m256i bbPixelMinX, bbPixelMinY, bbPixelMaxX, bbPixelMaxY;
		ComputeBoundingBox(bbPixelMinX, bbPixelMinY, bbPixelMaxX, bbPixelMaxY, pVtxX, pVtxY, scissor);

		// Clamp bounding box to tiles (it's already padded in computeBoundingBox)
		__m256i bbTileMinX = _mm256_srai_epi32(bbPixelMinX, TILE_WIDTH_SHIFT);
		__m256i bbTileMinY = _mm256_srai_epi32(bbPixelMinY, TILE_HEIGHT_SHIFT);
		__m256i bbTileMaxX = _mm256_srai_epi32(bbPixelMaxX, TILE_WIDTH_SHIFT);
		__m256i bbTileMaxY = _mm256_srai_epi32(bbPixelMaxY, TILE_HEIGHT_SHIFT);
		__m256i bbTileSizeX = _mm256_sub_epi32(bbTileMaxX, bbTileMinX);
		__m256i bbTileSizeY = _mm256_sub_epi32(bbTileMaxY, bbTileMinY);

		// Cull triangles with zero bounding box
		__m256i bboxSign = _mm256_or_si256(_mm256_sub_epi32(bbTileSizeX, _mm256_set1_epi32(1)), _mm256_sub_epi32(bbTileSizeY, _mm256_set1_epi32(1)));
		__m256 test321 = simd_cast<__m256>(bboxSign);
		unsigned int test_mask = triMask;
		triMask &= ~_mm256_movemask_ps(simd_cast<__m256>(bboxSign)) & SIMD_ALL_LANES_MASK;

		if (triMask == 0x0)
			return cullResult;

		if (!TEST_Z)
			cullResult = ECullingResult::VISIBLE;

		//////////////////////////////////////////////////////////////////////////////
		// Set up screen space depth plane
		//////////////////////////////////////////////////////////////////////////////

		__m256 zPixelDx, zPixelDy;
		ComputeDepthPlane(pVtxX, pVtxY, pVtxZ, zPixelDx, zPixelDy);

		// Compute z value at std::min corner of bounding box. Offset to make sure z is conservative for all 8x4 subtiles
		__m256 bbMinXV0 = _mm256_sub_ps(_mm256_cvtepi32_ps(bbPixelMinX), pVtxX[0]);
		__m256 bbMinYV0 = _mm256_sub_ps(_mm256_cvtepi32_ps(bbPixelMinY), pVtxY[0]);
		__m256 zPlaneOffset = _mm256_fmadd_ps(zPixelDx, bbMinXV0, _mm256_fmadd_ps(zPixelDy, bbMinYV0, pVtxZ[0]));
		__m256 zTileDx = _mm256_mul_ps(zPixelDx, _mm256_set1_ps((float)TILE_WIDTH));
		__m256 zTileDy = _mm256_mul_ps(zPixelDy, _mm256_set1_ps((float)TILE_HEIGHT));
		if (TEST_Z)//记录Zmax，也就是最近的深度值（reserve Z）
		{
			zPlaneOffset = _mm256_add_ps(zPlaneOffset, _mm256_max_ps(_mm256_setzero_ps(), _mm256_mul_ps(zPixelDx, _mm256_set1_ps(SUB_TILE_WIDTH))));
			zPlaneOffset = _mm256_add_ps(zPlaneOffset, _mm256_max_ps(_mm256_setzero_ps(), _mm256_mul_ps(zPixelDy, _mm256_set1_ps(SUB_TILE_HEIGHT))));
		}
		else//记录Zmin，也就是最远的深度值（reserve Z）
		{
			zPlaneOffset = _mm256_add_ps(zPlaneOffset, _mm256_min_ps(_mm256_setzero_ps(), _mm256_mul_ps(zPixelDx, _mm256_set1_ps(SUB_TILE_WIDTH))));
			zPlaneOffset = _mm256_add_ps(zPlaneOffset, _mm256_min_ps(_mm256_setzero_ps(), _mm256_mul_ps(zPixelDy, _mm256_set1_ps(SUB_TILE_HEIGHT))));
		}
		//zPlaneOffset is the std::max depth of this subtile which includes the std::min corner.
		// Compute Zmin and Zmax for the triangle (used to narrow the range for difficult tiles)
		__m256 zMin = _mm256_min_ps(pVtxZ[0], _mm256_min_ps(pVtxZ[1], pVtxZ[2]));
		__m256 zMax = _mm256_max_ps(pVtxZ[0], _mm256_max_ps(pVtxZ[1], pVtxZ[2]));

		//////////////////////////////////////////////////////////////////////////////
		// Sort vertices (v0 has lowest Y, and the rest is in winding order) and
		// compute edges. Also find the middle vertex and compute tile
		//////////////////////////////////////////////////////////////////////////////



		SortVertices(pVtxX, pVtxY);

		// Compute edges
		__m256 edgeX[3] = { _mm256_sub_ps(pVtxX[1], pVtxX[0]), _mm256_sub_ps(pVtxX[2], pVtxX[1]), _mm256_sub_ps(pVtxX[2], pVtxX[0]) };
		__m256 edgeY[3] = { _mm256_sub_ps(pVtxY[1], pVtxY[0]), _mm256_sub_ps(pVtxY[2], pVtxY[1]), _mm256_sub_ps(pVtxY[2], pVtxY[0]) };

		// Classify if the middle vertex is on the left or right and compute its position
		int midVtxRight = ~_mm256_movemask_ps(edgeY[1]);//把符号位全部移到最低的8个位，组成一个int
		__m256 midPixelX = _mm256_blendv_ps(pVtxX[1], pVtxX[2], edgeY[1]);
		__m256 midPixelY = _mm256_blendv_ps(pVtxY[1], pVtxY[2], edgeY[1]);
		__m256i midTileY = _mm256_srai_epi32(_mm256_max_epi32(_mm256_cvttps_epi32(midPixelY), SIMD_BITS_ZERO), TILE_HEIGHT_SHIFT);
		__m256i bbMidTileY = _mm256_max_epi32(bbTileMinY, _mm256_min_epi32(bbTileMaxY, midTileY));

		//////////////////////////////////////////////////////////////////////////////
		// Edge slope setup - Note we do not conform to DX/GL rasterization rules
		//////////////////////////////////////////////////////////////////////////////

		// Compute floating point slopes
		__m256 slope[3];
		slope[0] = _mm256_div_ps(edgeX[0], edgeY[0]);
		slope[1] = _mm256_div_ps(edgeX[1], edgeY[1]);
		slope[2] = _mm256_div_ps(edgeX[2], edgeY[2]);

		// Modify slope of horizontal edges to make sure they mask out pixels above/below the edge. The slope is set to screen
		// width to mask out all pixels above or below the horizontal edge. We must also add a small bias to acount for that 
		// vertices may end up off screen due to clipping. We're assuming that the round off error is no bigger than 1.0
		__m256 horizontalSlopeDelta = _mm256_set1_ps((float)width_ + 2.0f * (GUARD_BAND_PIXEL_SIZE + 1.0f));
		slope[0] = _mm256_blendv_ps(slope[0], horizontalSlopeDelta, _mm256_cmpeq_ps(edgeY[0], _mm256_setzero_ps()));
		slope[1] = _mm256_blendv_ps(slope[1], _mm256_neg_ps(horizontalSlopeDelta), _mm256_cmpeq_ps(edgeY[1], _mm256_setzero_ps()));

		// Convert floaing point slopes to fixed point
		__m256i slopeFP[3];
		slopeFP[0] = _mm256_cvttps_epi32(_mm256_mul_ps(slope[0], _mm256_set1_ps(1 << FP_BITS)));
		slopeFP[1] = _mm256_cvttps_epi32(_mm256_mul_ps(slope[1], _mm256_set1_ps(1 << FP_BITS)));
		slopeFP[2] = _mm256_cvttps_epi32(_mm256_mul_ps(slope[2], _mm256_set1_ps(1 << FP_BITS)));

		// Fan out edge slopes to avoid (rare) cracks at vertices. We increase right facing slopes 
		// by 1 LSB, which results in overshooting vertices slightly, increasing triangle coverage. 
		// e0 is always right facing, e1 depends on if the middle vertex is on the left or right
		slopeFP[0] = _mm256_add_epi32(slopeFP[0], _mm256_set1_epi32(1));
		slopeFP[1] = _mm256_add_epi32(slopeFP[1], _mm256_srli_epi32(_mm256_not_epi32(simd_cast<__m256i>(edgeY[1])), 31));

		// Compute slope deltas for an SIMD_LANES scanline step (tile height)
		__m256i slopeTileDelta[3];
		slopeTileDelta[0] = _mm256_slli_epi32(slopeFP[0], TILE_HEIGHT_SHIFT);
		slopeTileDelta[1] = _mm256_slli_epi32(slopeFP[1], TILE_HEIGHT_SHIFT);
		slopeTileDelta[2] = _mm256_slli_epi32(slopeFP[2], TILE_HEIGHT_SHIFT);

		// Compute edge events for the bottom of the bounding box, or for the middle tile in case of 
		// the edge originating from the middle vertex.
		__m256i xDiffi[2], yDiffi[2];
		xDiffi[0] = _mm256_slli_epi32(_mm256_sub_epi32(_mm256_cvttps_epi32(pVtxX[0]), bbPixelMinX), FP_BITS);
		xDiffi[1] = _mm256_slli_epi32(_mm256_sub_epi32(_mm256_cvttps_epi32(midPixelX), bbPixelMinX), FP_BITS);
		yDiffi[0] = _mm256_sub_epi32(_mm256_cvttps_epi32(pVtxY[0]), bbPixelMinY);
		yDiffi[1] = _mm256_sub_epi32(_mm256_cvttps_epi32(midPixelY), _mm256_slli_epi32(bbMidTileY, TILE_HEIGHT_SHIFT));

		__m256i eventStart[3];
		eventStart[0] = _mm256_sub_epi32(xDiffi[0], _mm256_mullo_epi32(slopeFP[0], yDiffi[0]));
		eventStart[1] = _mm256_sub_epi32(xDiffi[1], _mm256_mullo_epi32(slopeFP[1], yDiffi[1]));
		eventStart[2] = _mm256_sub_epi32(xDiffi[0], _mm256_mullo_epi32(slopeFP[2], yDiffi[0]));


		//////////////////////////////////////////////////////////////////////////////
		// Split bounding box into bottom - middle - top region.
		//////////////////////////////////////////////////////////////////////////////

		__m256i bbBottomIdx = _mm256_add_epi32(bbTileMinX, _mm256_mullo_epi32(bbTileMinY, _mm256_set1_epi32(tiles_width_)));
		__m256i bbTopIdx = _mm256_add_epi32(bbTileMinX, _mm256_mullo_epi32(_mm256_add_epi32(bbTileMinY, bbTileSizeY), _mm256_set1_epi32(tiles_width_)));
		__m256i bbMidIdx = _mm256_add_epi32(bbTileMinX, _mm256_mullo_epi32(midTileY, _mm256_set1_epi32(tiles_width_)));

		//////////////////////////////////////////////////////////////////////////////
		// Loop over non-culled triangle and change SIMD axis to per-pixel
		//////////////////////////////////////////////////////////////////////////////
		while (triMask)
		{
			unsigned int triIdx = find_clear_lsb(&triMask);
			int triMidVtxRight = (midVtxRight >> triIdx) & 1;

			// Get Triangle Zmin zMax
			__m256 zTriMax = _mm256_set1_ps(simd_f32(zMax)[triIdx]);
			__m256 zTriMin = _mm256_set1_ps(simd_f32(zMin)[triIdx]);

			// Setup Zmin value for first set of 8x4 subtiles
			__m256 z0 = _mm256_fmadd_ps(_mm256_set1_ps(simd_f32(zPixelDx)[triIdx]), SIMD_SUB_TILE_COL_OFFSET_F,
				_mm256_fmadd_ps(_mm256_set1_ps(simd_f32(zPixelDy)[triIdx]), SIMD_SUB_TILE_ROW_OFFSET_F, _mm256_set1_ps(simd_f32(zPlaneOffset)[triIdx])));

			float zx = simd_f32(zTileDx)[triIdx];
			float zy = simd_f32(zTileDy)[triIdx];

			// Get dimension of bounding box bottom, mid & top segments
			int bbWidth = simd_i32(bbTileSizeX)[triIdx];
			int bbHeight = simd_i32(bbTileSizeY)[triIdx];
			int tileRowIdx = simd_i32(bbBottomIdx)[triIdx];
			int tileMidRowIdx = simd_i32(bbMidIdx)[triIdx];
			int tileEndRowIdx = simd_i32(bbTopIdx)[triIdx];

			if (bbWidth > BIG_TRIANGLE&& bbHeight > BIG_TRIANGLE) // For big triangles we use a more expensive but tighter traversal algorithm
			{
				if (triMidVtxRight)
					cullResult &= RasterizeTriangle<TEST_Z, 1, 1>(triIdx, bbWidth, tileRowIdx, tileMidRowIdx, tileEndRowIdx, eventStart, slopeFP, slopeTileDelta, zTriMin, zTriMax, z0, zx, zy);
				else
					cullResult &= RasterizeTriangle<TEST_Z, 1, 0>(triIdx, bbWidth, tileRowIdx, tileMidRowIdx, tileEndRowIdx, eventStart, slopeFP, slopeTileDelta, zTriMin, zTriMax, z0, zx, zy);
			}
			else
			{
				if (triMidVtxRight)
					cullResult &= RasterizeTriangle<TEST_Z, 0, 1>(triIdx, bbWidth, tileRowIdx, tileMidRowIdx, tileEndRowIdx, eventStart, slopeFP, slopeTileDelta, zTriMin, zTriMax, z0, zx, zy);
				else
					cullResult &= RasterizeTriangle<TEST_Z, 0, 0>(triIdx, bbWidth, tileRowIdx, tileMidRowIdx, tileEndRowIdx, eventStart, slopeFP, slopeTileDelta, zTriMin, zTriMax, z0, zx, zy);
			}

			if (TEST_Z && cullResult == ECullingResult::VISIBLE)
				return ECullingResult::VISIBLE;
		}

		return cullResult;
	}
	void RenderTrilist(const ScissorRect* scissor)
	{
		assert(masked_hiz_buffer_ != nullptr);
		for (unsigned int i = 0; i < tri_list_.num_tri; i += SIMD_LANES)
		{
			//////////////////////////////////////////////////////////////////////////////
			// Fetch triangle vertices
			//////////////////////////////////////////////////////////////////////////////
			unsigned int numLanes = std::min((unsigned int)SIMD_LANES, tri_list_.num_tri - i);
			unsigned int triMask = (1U << numLanes) - 1;

			__m256 pVtxX[3], pVtxY[3], pVtxZ[3];

			for (unsigned int l = 0; l < numLanes; ++l)
			{
				unsigned int triIdx = i + l;
				for (int v = 0; v < 3; ++v)
				{
					simd_f32(pVtxX[v])[l] = tri_list_.ptr_tri[v * 3 + triIdx * 9 + 0];
					simd_f32(pVtxY[v])[l] = tri_list_.ptr_tri[v * 3 + triIdx * 9 + 1];
					simd_f32(pVtxZ[v])[l] = tri_list_.ptr_tri[v * 3 + triIdx * 9 + 2];
				}
			}
			//////////////////////////////////////////////////////////////////////////////
			// Setup and rasterize a SIMD batch of triangles
			//////////////////////////////////////////////////////////////////////////////
			RasterizeTriangleBatch<false>(pVtxX, pVtxY, pVtxZ, triMask, scissor);
		}
	}

	~MSOCManager()
	{
		occludee_bboxes_.clear();
		delete tri_list_.ptr_tri;
		delete[] masked_hiz_buffer_;
	}
private:
	void InitMSOCTriangleLists();

	/*member*/
	bool is_tri_list_initialized = false;
	std::vector<std::shared_ptr<Bbox>> occludee_bboxes_;
	TriList tri_list_;
	static MSOCManager* p_instance_;
	HakunaRenderer* ptr_renderer_;
	ZTile*			masked_hiz_buffer_;

	int             width_;
	int             height_;
	int             tiles_width_;
	int             tiles_height_;
};

