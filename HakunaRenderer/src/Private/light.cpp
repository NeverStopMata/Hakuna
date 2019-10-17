#include "light.h"



Light::Light()
{
}
Light::~Light()
{
}
glm::vec3 Light::GetScaledColor() {
	return color_ * intensity_;
}

