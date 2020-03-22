#pragma once
#include <memory>
#include "mesh.h"
#include "material.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include "bbox.h"
#include "msoc_mgr.h"
#include "cullable.h"
class HakunaRenderer;
class Transform
{
public:
	Transform():position_(glm::vec3(0,0,0)), scale_(glm::vec3(1,1,1)), rotation_axis_(glm::vec3(0, 1, 0)), rotate_angle_(0)
	{
		UpdateMatrices();
	}
	Transform(glm::vec3 position, glm::vec3 scale, glm::vec3 rotation_axis, float rotate_angle):
		position_(position), scale_(scale), rotation_axis_(glm::normalize(rotation_axis)), rotate_angle_(rotate_angle)
	{
		UpdateMatrices();
	}
	void UpdateTransform(const Transform& new_transform)
	{
		position_ = new_transform.position_;
		scale_ = new_transform.scale_;
		rotation_axis_ = new_transform.rotation_axis_;
		rotate_angle_ = new_transform.rotate_angle_;
		UpdateMatrices();
	}

	template<bool FOR_NORMAL>
	glm::mat4 GetModelMatrix()
	{
		return model_mats_[FOR_NORMAL ? 1 : 0];
	}

	glm::mat4* GetModelMatricesBeginPtr()
	{
		return model_mats_.data();
	}

	uint32_t GetMaticesSize()
	{
		return sizeof(model_mats_);
	}
private:
	void UpdateMatrices()
	{
		model_mats_[0] = CalculateModelMat();
		model_mats_[1] = glm::inverse(glm::transpose(model_mats_[0]));
	}
	glm::mat4 CalculateModelMat()
	{
		return  glm::translate(glm::mat4(1.0f), position_)* glm::rotate(glm::mat4(1.0f), rotate_angle_, rotation_axis_)* glm::scale(glm::mat4(1.0f), scale_);
	}

private:
	glm::vec3 position_;
	glm::vec3 scale_;
	glm::vec3 rotation_axis_;
	float rotate_angle_;
	std::array<glm::mat4,2> model_mats_;
};


enum class ERenderElementTtype { OccluderOnly, OccludeeOnly, OccluderAndOccludee  };
class RenderElement : public Cullable
{
public:
	const string mesh_resource_path_ = "resource/models/";
	const string mesh_postfix_ = ".obj";
	const string texture_resource_path_ = "resource/textures/";
	const string texture_postfix_ = ".dds";
	shared_ptr<Mesh> ptr_mesh_;
	ERenderElementTtype occlusion_type_;
	shared_ptr<Material> ptr_material_;
	HakunaRenderer* ptr_renderer_;

	vector<Triangle> occluder_triangles_;
	std::shared_ptr<Bbox> ptr_bbox_obj_space_;
	
	RenderElement(string name, Transform transform, HakunaRenderer* ptr_renderer, ERenderElementTtype occlusion_type);
	RenderElement() = delete;
	void CommitRendering(uint32_t currentImage);
	template<bool FOR_NORMAL>
	glm::mat4 GetModelMatrix()
	{
		return this->model_transform_.GetModelMatrix<FOR_NORMAL>();
	}

	bool IsNotOutOfViewFrustum()
	{
		return this->InsideOrIntersectViewFrustum();
	}

	void UpdateTransform(const Transform& new_transform, uint32_t currentImage);
private:
	Transform model_transform_;
	vector<VkDescriptorSet> descriptor_sets_;
	void CreateDescriptorSets();
	void InitOclusionInfo();
	void UpdateWorldSpaceAABB();
};

