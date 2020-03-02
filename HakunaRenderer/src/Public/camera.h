#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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
	~Camera() {}
	Camera(float fov_y, float aspect, float far, float near, vec3 position, vec3 lookat) :
		fov_y_(fov_y),
		aspect_(aspect),
		far_(far),
		near_(near),
		position_(position),
		forward_(glm::normalize(lookat - position_)),
		view_matrix_(lookAt(position_, lookat, up_)),
		up_(cross(cross(forward_, vec3(0, 1, 0)), forward_)),
		proj_matrix_(perspective(radians(fov_y_), aspect_, near_, far_)),
		forward_move_direction_(0),
		right_move_direction_(0),
		move_speed_(1.f),
		is_rotating_(false),
		pitch_speed_(0),
		yaw_speed_(0),
		is_speed_up(false),
		is_slow_down(false){}

	mat4x4 GetViewMatrix() const{
		return view_matrix_;
	}
	mat4x4 GetProjMatrix() const{
		return proj_matrix_;
	}
	vec3 GetWorldPos() {
		return position_;
	}
	void UpdateAspect(float aspect) {
		aspect_ = aspect;
		proj_matrix_ = perspective(radians(fov_y_), aspect_, near_, far_);
	}
	vec3 GetForwardVector() const {
		return forward_;
	}
	vec3 GetUpVector() const {
		return up_;
	}
	vec3 GetRightVector() const {
		return glm::cross(forward_, up_);
	}
	float GetNear() const
	{
		return near_;
	}
	float GetFar() const
	{
		return far_;
	}
	float GetFovYInRadians() const {
		return radians(fov_y_);
	}
	float GetAspect() const {
		return aspect_;
	}
	void UpdatePerFrame(float deltaTime);
	void SetupCameraContrl(InputManager& input_mgr);
	void ReleaseCameraContrl(InputManager& input_mgr);
private:
	float fov_y_;
	float aspect_;
	float far_;
	float near_;
	vec3 position_;
	vec3 forward_;
	vec3 up_;
	float forward_move_direction_;
	float right_move_direction_;
	float move_speed_;
	mat4x4 view_matrix_;
	mat4x4 proj_matrix_;
	bool is_rotating_;
	double pitch_speed_;
	double yaw_speed_;
	bool is_speed_up;
	bool is_slow_down;
	void UpdateViewMatrix()
	{
		view_matrix_ = lookAt(position_, position_ + forward_, up_);
	}
	float GetAdjustSpeed()
	{
		return (is_slow_down ? 0.05f : (is_speed_up ? 5.0f : 1.0f));
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

	void SetSpeedUp(bool val)
	{
		is_speed_up = val;
	}

	void SetSlowDown(bool val)
	{
		is_slow_down = val;
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

