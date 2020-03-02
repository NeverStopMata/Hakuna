#pragma once
#include <functional>
#include <map>
#include <vector>
#include <GLFW/glfw3.h>
#include <iostream>
class InputManager
{
	struct inputMode
	{
		int key;
		int action;
		bool operator < (inputMode const& _A) const
		{
			//这个函数指定排序策略，按nID排序，如果nID相等的话，按strName排序
			if (key < _A.key)  return true;
			if (key == _A.key) return action < _A.action;
			return false;
		}
	};
public:
	using Callback = std::function<void()>;
	void Init(GLFWwindow* window);
	void AddKeyBoardBinding(int key, int action, const Callback& callback);
	void AddMouseButtonBinding(int key, int action, const Callback& callback);
	void SetMouseAxisBinding(double* x_axis, double* y_axis);

	void RemoveKeyBoardBinding(int key, int action, const Callback& callback);
	void RemoveMouseButtonBinding(int key, int action, const Callback& callback);
	void ClearMouseAxisBinding();
	bool CheckKeyState(int key, int state);
private:
	std::map<inputMode, std::vector<Callback>> key_board_callbacks_;

	std::map<inputMode, std::vector<Callback>> mouse_button_callbacks_;

	GLFWwindow* window_;
	double last_cursor_pos_x, last_cursor_pos_y;
	double* x_axis_,* y_axis_;
	float mouse_speed_;
	static void KeyBoardCallbackStatic(GLFWwindow* window,
		int key,
		int scancode,
		int action,
		int mods);
	void KeyBoardCallback(GLFWwindow* window,
		int key,
		int scancode,
		int action,
		int mods);

	static void MouseButtonCallbackStatic(GLFWwindow* window,
		int button,
		int action,
		int mods);
	void MouseButtonCallback(GLFWwindow* window,
		int button,
		int action,
		int mods);

	static void MouseMoveCallbackStatic(GLFWwindow* window, double xpos, double ypos);
	void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
};

