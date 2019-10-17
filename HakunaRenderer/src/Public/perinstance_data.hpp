#pragma once
#include <glm/glm.hpp>
#include <iostream>
struct PerInstanceDataBase
{
	glm::mat4x4 MtoW;
};

struct PerInstanceDataWithLightmap : public PerInstanceDataBase
{
	glm::vec4 lightmap_offset_scale_;
	uint32_t lightmap_index_;
	void print_content()
	{
		std::cout << lightmap_offset_scale_.x << std::endl;
		std::cout << lightmap_index_ << std::endl;
		std::cout << MtoW[0][0] << std::endl;
	}
};
