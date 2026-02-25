//
// Created by rodrigo.reis on 06/11/24.
//

#ifndef _MESHUTILS_H_
#define _MESHUTILS_H_

#include "core/ncsb/component.h"
#include "imp.h"

 class MeshUtils: public imp::Component
{
 public:
	void Setup();
	void Setup(imp::AssetPtr<imp::ImageAsset> textureAsset, imp::AssetPtr<imp::ImageAsset> maskAsset, imp::AssetPtr<imp::ImageAsset> animationAsset, imp::AssetPtr<imp::MaterialAsset> materialAsset);
	imp::NodeHandle CreateProceduralQuad(imp::float2 size, imp::float2 center);

 private:
	imp::Material* material_;
	imp::AssetPtr<imp::ImageAsset> texture_asset_;
	imp::AssetPtr<imp::ImageAsset> mask_asset_;
	imp::AssetPtr<imp::ImageAsset> animation_asset_;
	imp::AssetPtr<imp::MaterialAsset> material_asset_;
	imp::float3 color_;
	imp::float2 alpha_range_;
	float fade_speed_;
	std::string name_;
};

#endif //_MESHUTILS_H_
