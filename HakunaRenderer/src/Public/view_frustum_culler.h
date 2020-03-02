#pragma once
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include "thread_pool.h"
class Cullable;
struct Plane
{
	glm::vec3 normal_;
	float delta_;
};

class Camera;
class RenderElement;
class BBox;
enum class EViewFrustumCullResult {Outside,InsideOrIntersect};
class ViewFrustumCuller
{
public:
	ViewFrustumCuller() :thread_pool_(6), owner_(nullptr){};
	void SetOwnerCamera(Camera* owner_ptr);
	void DoCull(std::vector<RenderElement>& render_elements);
private:
	void UpdateFrustumVolumePlanesAndCenterFrame();
	bool CheckAABBInsideFrustumVolume(Cullable * cullee);
	bool IsAABBOutsidePlane(uint32_t plane_idx, Cullable& cullee);
private:
	static std::array<std::array<int,3>,8> test_planes_lut_;
	Camera* owner_;
	std::array<Plane, 6> view_frustum_planes_;//near far up down left right.
	glm::vec3 frustum_volume_center_;
	std::array<glm::vec3, 3> frustum_center_frame_;
	ThreadPool thread_pool_;
};