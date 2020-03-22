#pragma once
#include <glm/glm.hpp>
#include <limits>

class Bbox
{
	friend class Mesh;
	glm::vec3 max_;
	glm::vec3 min_;
public:
	Bbox():max_(glm::vec3(-std::numeric_limits<float>::max())),min_(glm::vec3(std::numeric_limits<float>::max()))
	{
	}
	glm::vec3 GetMax()const { return max_; };
	glm::vec3 GetMin()const { return min_; };
	glm::vec3 GetCenter()const { return (min_ + max_)*0.5f; };
	void SetMax(glm::vec3 max_val) { max_ = max_val;  };
	void SetMin(glm::vec3 min_val) { min_ = min_val; };
	/*float max_x_;
	float max_y_;
	float max_z_;
	float min_x_;
	float min_y_;
	float min_z_;*/
};
