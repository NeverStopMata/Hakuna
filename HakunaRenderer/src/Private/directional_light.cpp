#include "directional_light.h"



DirectionalLight::DirectionalLight()
{
}
DirectionalLight::~DirectionalLight()
{
}
glm::vec3 DirectionalLight::GetDirection() {
	return glm::normalize(direction_);
}
