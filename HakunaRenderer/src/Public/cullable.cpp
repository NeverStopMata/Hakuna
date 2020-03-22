#include "cullable.h"

const Bbox& Cullable::GetAABBInWorldSpace() const
{
	return aabb_world_space_;
}
