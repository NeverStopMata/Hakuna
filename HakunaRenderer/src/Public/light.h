#pragma once
#include <glm/glm.hpp>
class Light
{
public:
	Light();
	Light(glm::vec3 color, float intensity):color_(color),intensity_(intensity){}
	virtual ~Light();
	glm::vec3 GetScaledColor();
public:
	glm::vec3 color_;
	float intensity_;
};

