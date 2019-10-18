#pragma once
#include <memory>
#include "mesh.h"
#include "material.h"
class Model
{
public:
	std::shared_ptr<Mesh> mesh_;
	std::shared_ptr<Material> material_;
};

