#include "view_frustum_culler.h"
#include "camera.h"
#include "cullable.h"
#include "render_element.h"
#include "bbox.h"
#include <iostream>
//test only three planes.
std::array<std::array<int, 3>, 8> ViewFrustumCuller::test_planes_lut_ = { 4,3,4, 4,3,0, 4,2,1, 4,2,0, 5,3,1, 5,3,0, 5,2,1, 5,2,0 };
void ViewFrustumCuller::SetOwnerCamera(Camera* owner_ptr)
{
	owner_ = owner_ptr;
}
void ViewFrustumCuller::DoCull(std::vector<RenderElement>& render_elements)
{
	UpdateFrustumVolumePlanesAndCenterFrame();
	std::vector< std::future<bool> > results;
	for (int i = 0; i < render_elements.size(); ++i) {
		Cullable* ptr_cullee = dynamic_cast<Cullable*>(&(render_elements[i]));
		results.emplace_back(
			thread_pool_.enqueue([ptr_cullee, this] {
			return CheckAABBInsideFrustumVolume(ptr_cullee);
		})
		);
	}

	for (auto&& result : results)
		result.get();

}

void ViewFrustumCuller::UpdateFrustumVolumePlanesAndCenterFrame()
{
	glm::vec3 forward_vec = owner_->GetForwardVector();
	view_frustum_planes_[0].normal_ = -forward_vec;
	view_frustum_planes_[1].normal_ = forward_vec;
	vec3 camera_pos = owner_->GetWorldPos();
	glm::vec3 near_plane_center = camera_pos + forward_vec * owner_->GetNear();
	glm::vec3 far_plane_center = camera_pos + forward_vec * owner_->GetFar();
	view_frustum_planes_[0].delta_ = glm::dot(view_frustum_planes_[0].normal_, near_plane_center);
	view_frustum_planes_[1].delta_ = glm::dot(view_frustum_planes_[1].normal_, far_plane_center);
	float far_plane_halfH = glm::tan(owner_->GetFovYInRadians() * 0.5f) * owner_->GetFar();
	float far_plane_halfW = far_plane_halfH * owner_->GetAspect();
	vec3 up_vec = owner_->GetUpVector();
	vec3 far_plane_top_center = far_plane_center + up_vec * far_plane_halfH;
	vec3 far_plane_bottom_center = far_plane_center - up_vec * far_plane_halfH;
	vec3 right_vec = owner_->GetRightVector();
	vec3 far_plane_right_center = far_plane_center + right_vec * far_plane_halfW;
	vec3 far_plane_left_center = far_plane_center - right_vec * far_plane_halfW;
	view_frustum_planes_[2].normal_ = normalize(cross(far_plane_top_center - camera_pos, -right_vec));
	view_frustum_planes_[3].normal_ = normalize(cross(far_plane_bottom_center - camera_pos, right_vec));
	view_frustum_planes_[4].normal_ = normalize(cross(far_plane_left_center - camera_pos, -up_vec));
	view_frustum_planes_[5].normal_ = normalize(cross(far_plane_right_center - camera_pos, up_vec));
	for (int i = 2; i < 6; i++)
	{
		view_frustum_planes_[i].delta_ = glm::dot(view_frustum_planes_[i].normal_, camera_pos);
	}
	frustum_center_frame_[0] = right_vec;
	frustum_center_frame_[1] = up_vec;
	frustum_center_frame_[2] = -forward_vec;
	frustum_volume_center_ = (near_plane_center + far_plane_center) * 0.5f;
}

bool ViewFrustumCuller::CheckAABBInsideFrustumVolume(Cullable* cullee)
{
	if (IsAABBOutsidePlane(cullee->last_outside_plane_idx_, *cullee))
	{
		cullee->is_outside_viewfrustum_ = true;
		return false;
	}
	glm::vec3 max_pos = cullee->aabb_world_space_.GetMax();
	glm::vec3 min_pos = cullee->aabb_world_space_.GetMin();
	glm::vec3 aabb_center = (min_pos + max_pos) * 0.5f;
	glm::vec3 frustum_center_to_aabb_center = aabb_center - frustum_volume_center_;
	uint32_t sign_bits = dot(frustum_center_frame_[2], frustum_center_to_aabb_center) > 0 ? 1 : 0;
	sign_bits += dot(frustum_center_frame_[1], frustum_center_to_aabb_center) > 0 ? 2 : 0;
	sign_bits += dot(frustum_center_frame_[0], frustum_center_to_aabb_center) > 0 ? 4 : 0;
	for (uint32_t i = 0; i < 3; i++)
	{
		uint32_t plane_idx = test_planes_lut_[sign_bits][i];
		if (cullee->last_outside_plane_idx_ == plane_idx) { continue; }
		if (IsAABBOutsidePlane(plane_idx, *cullee))
		{
			cullee->is_outside_viewfrustum_ = true;
			cullee->last_outside_plane_idx_ = plane_idx;
			return false;
		}
	}
	cullee->is_outside_viewfrustum_ = false;
	return true;
}

bool ViewFrustumCuller::IsAABBOutsidePlane(uint32_t plane_idx, Cullable& cullee)
{
	glm::vec3 max_pos = cullee.aabb_world_space_.GetMax();
	glm::vec3 min_pos = cullee.aabb_world_space_.GetMin();
	glm::vec3 plane_normal = view_frustum_planes_[plane_idx].normal_;
	glm::vec3 negetive_point = glm::vec3(
		plane_normal.x > 0 ? min_pos.x : max_pos.x,
		plane_normal.y > 0 ? min_pos.y : max_pos.y,
		plane_normal.z > 0 ? min_pos.z : max_pos.z);
	return glm::dot(plane_normal, negetive_point) > view_frustum_planes_[plane_idx].delta_;
}
