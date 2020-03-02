#pragma once
#include "bbox.h"
class Cullable
{
friend class ViewFrustumCuller;

public:
	
protected:
	Bbox aabb_world_space_;
	bool NeedRender()
	{
		return !is_outside_viewfrustum_;
	}
private:
	int last_outside_plane_idx_;
	bool is_outside_viewfrustum_ = false;
};