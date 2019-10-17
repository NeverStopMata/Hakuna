#pragma once
#include <memory>
#include "model.h"
#include "perinstance_data.hpp"
template<typename PerInstanceDataType>
class DrawItem
{
public:
	std::shared_ptr<Model> model_;
	PerInstanceDataType instance_data_;


	DrawItem() {
		model_ = nullptr;
		instance_data_.MtoW = glm::mat4(1.0);
	}
	void print_instance_data()
	{
		instance_data_.print_content();
	}
};

