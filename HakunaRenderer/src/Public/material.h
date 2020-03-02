
#include "texture_mgr.h"
#include <memory>
class Material
{
public:
	const array<string, 5> texture_types_ = { "basecolor" ,"normal","metallic","roughness","occlusion" };
	enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
	AlphaMode alphaMode = ALPHAMODE_OPAQUE;
	vector<string> material_tex_names_;
	Material(string name)
	{
		for (auto tex_type : texture_types_)
		{
			material_tex_names_.emplace_back(name + "_" + tex_type);
		}
		material_tex_names_.emplace_back("env_irradiance_cubemap");
		material_tex_names_.emplace_back("env_specular_cubemap");
		material_tex_names_.emplace_back("env_brdf_lut");
	}
};