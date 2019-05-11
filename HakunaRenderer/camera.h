#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;
class Camera
{
public:
	Camera(float fov_y, float aspect, float far, float near, vec3 position, vec3 lookat) {
		fov_y_ = fov_y;
		aspect_ = aspect;
		far_ = far;
		near_ = near;
		position_ = position;
		lookat_ = lookat;
		up_ = vec3(0, 1, 0);
		focus_dist_ = (lookat_ - position_).length();
		view_matrix_ = lookAt(position_, lookat_, up_);
		proj_matrix_ = perspective(radians(fov_y_), aspect_, near_, far_);
	}
	mat4x4 GetViewMatrix() {
		return view_matrix_;
	}
	mat4x4 GetProjMatrix() {
		return proj_matrix_;
	}
	~Camera() {}
private:
	float fov_y_;
	float aspect_;
	float far_;
	float near_;
	vec3 position_;
	vec3 lookat_;
	vec3 up_;
	float focus_dist_;
	mat4x4 view_matrix_;
	mat4x4 proj_matrix_;
};

