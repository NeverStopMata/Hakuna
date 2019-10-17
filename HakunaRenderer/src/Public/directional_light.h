#pragma once
#include "light.h"
class DirectionalLight :public Light{
public:
	DirectionalLight(glm::vec3 dir, glm::vec3 color, float intensity) :Light(color, intensity), direction_(dir) {};
	DirectionalLight();
	~DirectionalLight();
	glm::vec3 GetDirection();

public:
	glm::vec3 direction_;
};

