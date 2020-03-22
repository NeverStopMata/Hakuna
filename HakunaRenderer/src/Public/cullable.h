#pragma once
#include "bbox.h"
class Cullable
{
friend class ViewFrustumCuller;

public:
	const Bbox& GetAABBInWorldSpace()const;
	void SetIsOccluded(bool&& val)
	{
		is_occluded = val;
	}
protected:
	Bbox aabb_world_space_;	
	bool InsideOrIntersectViewFrustum()
	{
		return !is_outside_viewfrustum_;
	}

	bool IsOccluded()
	{
		return is_occluded;
	}
private:
	int last_outside_plane_idx_;
	bool is_outside_viewfrustum_ = false;
	bool is_occluded = false;
};