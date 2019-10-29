
#include "texture_mgr.h"
#include <memory>
class Material
{
	enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
	AlphaMode alphaMode = ALPHAMODE_OPAQUE;
	std::shared_ptr<Texture> baseColorTexture;
	std::shared_ptr<Texture> metallicTexture;
	std::shared_ptr<Texture> normalTexture;
	std::shared_ptr<Texture> occlusionTexture;
	std::shared_ptr<Texture> roughnessTexture;
	std::shared_ptr<Texture> emissiveTexture;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};