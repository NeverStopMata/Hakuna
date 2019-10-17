#include "camera.h"

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

	/*=========================================================*/
	input_mgr.RemoveMouseButtonBinding(GLFW_MOUSE_BUTTON_2, GLFW_PRESS, [=] {
		SetIsRotating(true);
	});

	input_mgr.RemoveMouseButtonBinding(GLFW_MOUSE_BUTTON_2, GLFW_RELEASE, [=] {
		SetIsRotating(false);
	});
}
