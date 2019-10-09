#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "input_mgr.h"
#include "GLFW/glfw3.h"
#include <math.h>
using namespace glm;
class Camera
{
public:
	Camera() {}
	Camera(float fov_y, float aspect, float far, float near, vec3 position, vec3 lookat) {
		fov_y_ = fov_y;
		aspect_ = aspect;
		far_ = far;
		near_ = near;
		position_ = position;
		up_ = vec3(0, 1, 0);
		forward_ = lookat - position_;
		focus_dist_ = forward_.length();
		forward_ /= focus_dist_;
		view_matrix_ = lookAt(position_, lookat, up_);
		glm::vec3 right = cross(forward_, up_);
		up_ = cross(right, forward_);
		proj_matrix_ = perspective(radians(fov_y_), aspect_, near_, far_);
		forward_move_direction_ = 0;
		right_move_direction_ = 0;
		move_speed_ = 0.0002f;
		is_rotating_ = false;
	}

	mat4x4 GetViewMatrix() {
		return view_matrix_;
	}
	mat4x4 GetProjMatrix() {
		return proj_matrix_;
	}
	vec3 GetWorldPos() {
		return position_;
	}
	void UpdateAspect(float aspect) {
		aspect_ = aspect;
		proj_matrix_ = perspective(radians(fov_y_), aspect_, near_, far_);
	}

	void UpdatePerFrame(float deltaTime)
	{
		bool needUpdateViewMatrix = false;
		if (forward_move_direction_ != 0)
		{
			position_ += forward_ * move_speed_ * forward_move_direction_ * deltaTime;
			needUpdateViewMatrix = true;
		}

		if (right_move_direction_ != 0)
		{
			position_ += cross(forward_,up_) * move_speed_ * right_move_direction_ * deltaTime;
			needUpdateViewMatrix = true;
		}

		if (is_rotating_)
		{
			if (yaw_speed_ != 0)
			{
				RotateEular(yaw_speed_ * deltaTime, glm::vec3(0,1,0));
				needUpdateViewMatrix = true;
				yaw_speed_ = 0;
			}
			if (pitch_speed_ != 0)
			{
				RotateEular(pitch_speed_ * deltaTime, cross(forward_,up_));
				needUpdateViewMatrix = true;
				pitch_speed_ = 0;
			}
		}
		if (needUpdateViewMatrix)
		{
			UpdateViewMatrix();
		}
	}
	void SetupCameraContrl(InputManager& input_mgr);
	void ReleaseCameraContrl(InputManager& input_mgr);
	~Camera() {}
private:
	float fov_y_;
	float aspect_;
	float far_;
	float near_;
	vec3 position_;
	vec3 forward_;
	vec3 up_;
	float focus_dist_;
	float forward_move_direction_;
	float right_move_direction_;
	float move_speed_;
	mat4x4 view_matrix_;
	mat4x4 proj_matrix_;
	bool is_rotating_;
	double pitch_speed_;
	double yaw_speed_;
	void UpdateViewMatrix()
	{
		view_matrix_ = lookAt(position_, position_ + forward_, up_);
	}

	void AddForwardSpeed(float forwardSpeedDelta)
	{
		forward_move_direction_ += forwardSpeedDelta;
		forward_move_direction_ = glm::clamp(forward_move_direction_, -1.0f, 1.0f);
	}

	void AddRightSpeed(float forwardSpeedDelta)
	{
		right_move_direction_ += forwardSpeedDelta;
		right_move_direction_ = glm::clamp(right_move_direction_, -1.0f, 1.0f);
	}

	void RotateEular(float rotate_angle,glm::vec3 rotate_aixs)
	{
		auto rot_mat = glm::rotate(
			glm::mat4(1.0f),
			rotate_angle,
			rotate_aixs);
		up_ = vec3(rot_mat * glm::vec4(up_,0));
		forward_ = vec3(rot_mat * glm::vec4(forward_, 0));
	}
	void SetIsRotating(bool is_rotate)
	{
		is_rotating_ = is_rotate;
	}
};

