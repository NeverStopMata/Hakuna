#include "camera.h"

void Camera::UpdatePerFrame(float deltaTime)
{
	bool needUpdateViewMatrix = false;
	if (forward_move_direction_ != 0)
	{
		position_ += forward_ * move_speed_ * forward_move_direction_ * GetAdjustSpeed() * deltaTime;
		needUpdateViewMatrix = true;
	}

	if (right_move_direction_ != 0)
	{
		position_ += cross(forward_, up_) * move_speed_ * right_move_direction_ * GetAdjustSpeed() * deltaTime;
		needUpdateViewMatrix = true;
	}

	if (is_rotating_)
	{
		if (yaw_speed_ != 0)
		{
			RotateEular(yaw_speed_ * deltaTime, glm::vec3(0, 1, 0));
			needUpdateViewMatrix = true;
			yaw_speed_ = 0;
		}
		if (pitch_speed_ != 0)
		{
			RotateEular(pitch_speed_ * deltaTime, cross(forward_, up_));
			needUpdateViewMatrix = true;
			pitch_speed_ = 0;
		}
	}
	if (needUpdateViewMatrix)
	{
		UpdateViewMatrix();
	}
}

void Camera::SetupCameraContrl(InputManager& input_mgr)
{
	input_mgr.SetMouseAxisBinding(&this->yaw_speed_, &this->pitch_speed_);
	input_mgr.AddKeyBoardBinding(GLFW_KEY_W, GLFW_PRESS, [=] {
		AddForwardSpeed(1);
	});

	input_mgr.AddKeyBoardBinding(GLFW_KEY_W, GLFW_RELEASE, [=] {
		AddForwardSpeed(-1);
	});

	input_mgr.AddKeyBoardBinding(GLFW_KEY_S, GLFW_PRESS, [=] {
		AddForwardSpeed(-1);
	});

	input_mgr.AddKeyBoardBinding(GLFW_KEY_S, GLFW_RELEASE, [=] {
		AddForwardSpeed(1);
	});

	input_mgr.AddKeyBoardBinding(GLFW_KEY_D, GLFW_PRESS, [=] {
		AddRightSpeed(1);
	});

	input_mgr.AddKeyBoardBinding(GLFW_KEY_D, GLFW_RELEASE, [=] {
		AddRightSpeed(-1);
	});

	input_mgr.AddKeyBoardBinding(GLFW_KEY_A, GLFW_PRESS, [=] {
		AddRightSpeed(-1);
	});

	input_mgr.AddKeyBoardBinding(GLFW_KEY_A, GLFW_RELEASE, [=] {
		AddRightSpeed(1);
	});

	input_mgr.AddKeyBoardBinding(GLFW_KEY_LEFT_SHIFT, GLFW_PRESS, [=] {
		SetSpeedUp(true);
	});

	input_mgr.AddKeyBoardBinding(GLFW_KEY_LEFT_SHIFT, GLFW_RELEASE, [=] {
		SetSpeedUp(false);
	});

	input_mgr.AddKeyBoardBinding(GLFW_KEY_LEFT_CONTROL, GLFW_PRESS, [=] {
		SetSlowDown(true);
	});

	input_mgr.AddKeyBoardBinding(GLFW_KEY_LEFT_CONTROL, GLFW_RELEASE, [=] {
		SetSlowDown(false);
	});
	/*=========================================================*/
	input_mgr.AddMouseButtonBinding(GLFW_MOUSE_BUTTON_2, GLFW_PRESS, [=] {
		SetIsRotating(true);
	});

	input_mgr.AddMouseButtonBinding(GLFW_MOUSE_BUTTON_2, GLFW_RELEASE, [=] {
		SetIsRotating(false);
	});


}

void Camera::ReleaseCameraContrl(InputManager& input_mgr)
{
	input_mgr.ClearMouseAxisBinding();
	/*===================================================*/
	input_mgr.RemoveKeyBoardBinding(GLFW_KEY_W, GLFW_PRESS, [=] {
		AddForwardSpeed(1);
	});

	input_mgr.RemoveKeyBoardBinding(GLFW_KEY_W, GLFW_RELEASE, [=] {
		AddForwardSpeed(-1);
	});

	input_mgr.RemoveKeyBoardBinding(GLFW_KEY_S, GLFW_PRESS, [=] {
		AddForwardSpeed(-1);
	});

	input_mgr.RemoveKeyBoardBinding(GLFW_KEY_S, GLFW_RELEASE, [=] {
		AddForwardSpeed(1);
	});

	/*========================================================*/
	input_mgr.RemoveKeyBoardBinding(GLFW_KEY_D, GLFW_PRESS, [=] {
		AddRightSpeed(1);
	});

	input_mgr.RemoveKeyBoardBinding(GLFW_KEY_D, GLFW_RELEASE, [=] {
		AddRightSpeed(-1);
	});

	input_mgr.RemoveKeyBoardBinding(GLFW_KEY_A, GLFW_PRESS, [=] {
		AddRightSpeed(-1);
	});

	input_mgr.RemoveKeyBoardBinding(GLFW_KEY_A, GLFW_RELEASE, [=] {
		AddRightSpeed(1);
	});

	input_mgr.RemoveKeyBoardBinding(GLFW_KEY_LEFT_SHIFT, GLFW_PRESS, [=] {
		SetSpeedUp(true);
	});

	input_mgr.RemoveKeyBoardBinding(GLFW_KEY_LEFT_SHIFT, GLFW_RELEASE, [=] {
		SetSpeedUp(false);
	});

	input_mgr.RemoveKeyBoardBinding(GLFW_KEY_LEFT_CONTROL, GLFW_PRESS, [=] {
		SetSlowDown(true);
	});

	input_mgr.RemoveKeyBoardBinding(GLFW_KEY_LEFT_CONTROL, GLFW_RELEASE, [=] {
		SetSlowDown(false);
	});

	/*=========================================================*/
	input_mgr.RemoveMouseButtonBinding(GLFW_MOUSE_BUTTON_2, GLFW_PRESS, [=] {
		SetIsRotating(true);
	});

	input_mgr.RemoveMouseButtonBinding(GLFW_MOUSE_BUTTON_2, GLFW_RELEASE, [=] {
		SetIsRotating(false);
	});

	/*=========================================================*/

}
