#include "input_mgr.h"


void InputManager::Init(GLFWwindow* window)
{
	window_ = window;
	x_axis_ = nullptr;
	y_axis_ = nullptr;
	mouse_speed_ = 0.03;
	
	if (window_)
	{
		glfwGetCursorPos(window_, &last_cursor_pos_x, &last_cursor_pos_y);
		glfwSetWindowUserPointer(window_, this);
		glfwSetKeyCallback(window_, KeyBoardCallbackStatic);
		glfwSetMouseButtonCallback(window_, MouseButtonCallbackStatic);
		glfwSetCursorPosCallback(window_, MouseMoveCallbackStatic);		
	}
	else
	{
		throw std::runtime_error("input manager's window is nullptr");
	}

}

void InputManager::AddKeyBoardBinding(int key, int action, const Callback& callback)
{
	key_board_callbacks_[inputMode{ key,action }].push_back(callback);
}

void InputManager::AddMouseButtonBinding(int key, int action, const Callback& callback)
{
	mouse_button_callbacks_[inputMode{ key,action }].push_back(callback);
}

void InputManager::SetMouseAxisBinding(double* x_axis, double* y_axis)
{
	x_axis_ = x_axis;
	y_axis_ = y_axis;
}

void InputManager::RemoveKeyBoardBinding(int key, int action, const Callback& callback)
{
	std::vector<Callback>::iterator to_delete_callback;
	int offset = -1;
	auto delete_func_ptr = callback.target<void(*)()>();
	for (auto tmp_callback: key_board_callbacks_[inputMode{ key ,action }])
	{
		offset++;
		auto tmp_func_ptr = tmp_callback.target<void(*)()>();
		if (tmp_func_ptr == delete_func_ptr)
		{
			break;
		}
	}
	to_delete_callback = key_board_callbacks_[inputMode{ key ,action }].begin() + offset;
	key_board_callbacks_[inputMode{ key ,action }].erase(to_delete_callback);
}

void InputManager::RemoveMouseButtonBinding(int key, int action, const Callback& callback)
{
	std::vector<Callback>::iterator to_delete_callback;
	int offset = -1;
	auto delete_func_ptr = callback.target<void(*)()>();
	for (auto tmp_callback : mouse_button_callbacks_[inputMode{ key ,action }])
	{
		offset++;
		auto tmp_func_ptr = tmp_callback.target<void(*)()>();
		if (tmp_func_ptr == delete_func_ptr)
		{
			break;
		}
	}
	to_delete_callback = mouse_button_callbacks_[inputMode{ key ,action }].begin() + offset;
	mouse_button_callbacks_[inputMode{ key ,action }].erase(to_delete_callback);
}

void InputManager::ClearMouseAxisBinding()
{
	x_axis_ = nullptr;
	y_axis_ = nullptr;
}

//keyboard callback
void InputManager::KeyBoardCallbackStatic(GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mods)
{
	InputManager* that = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
	that->KeyBoardCallback(window, key, scancode, action, mods);
}


void InputManager::KeyBoardCallback(GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mods)
{
	for (Callback& callback : key_board_callbacks_[inputMode{key,action}])
	{
		callback();
	}
}

//mouse button callback
void InputManager::MouseButtonCallbackStatic(GLFWwindow* window,
	int button,
	int action,
	int mods)
{
	InputManager* that = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
	that->MouseButtonCallback(window, button, action, mods);
}


void InputManager::MouseButtonCallback(GLFWwindow* window,
	int button,
	int action,
	int mods)
{
	if (action == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}
	else if(action == GLFW_RELEASE)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
	for (Callback& callback : mouse_button_callbacks_[inputMode{ button,action }])
	{
		callback();
	}
}

//mouse move callback
void InputManager::MouseMoveCallbackStatic(GLFWwindow* window, double xpos, double ypos)
{
	InputManager* that = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
	that->MouseMoveCallback(window, xpos, ypos);
}

void InputManager::MouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (y_axis_!=nullptr && x_axis_ != nullptr)
	{
		int width, height;
		glfwGetWindowSize(window, &width, &height);

		*x_axis_ = -((xpos - last_cursor_pos_x)*2.0 / static_cast<double>(width)) * mouse_speed_;
		*y_axis_ = -((ypos - last_cursor_pos_y)*2.0 / static_cast<double>(height)) * mouse_speed_;

		if ((xpos >= width-1 || xpos <= 0 || ypos >= height-1 || ypos <= 0)&&(glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_HIDDEN))
		{
			last_cursor_pos_x = double(width) / 2.0;
			last_cursor_pos_y = double(height) / 2.0f;
			glfwSetCursorPos(window, last_cursor_pos_x, last_cursor_pos_y);
		}
		else
		{
			last_cursor_pos_x = xpos;
			last_cursor_pos_y = ypos;
		}
	}
}
