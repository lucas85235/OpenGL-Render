//
// Created by rodrigo.reis on 06/11/24.
//

#include <random>
#include <utility>
#include "MeshUtils.h"

using namespace imp;

void MeshUtils::Setup()
{
	Component::Setup();
}

void MeshUtils::Setup(imp::AssetPtr<imp::ImageAsset> textureAsset, imp::AssetPtr<imp::ImageAsset> maskAsset, imp::AssetPtr<imp::ImageAsset> animationAsset, imp::AssetPtr<imp::MaterialAsset> materialAsset)
{
	texture_asset_ = std::move(textureAsset);
	mask_asset_ = std::move(maskAsset);
	material_asset_ = std::move(materialAsset);
	animation_asset_= animationAsset;
}

imp::NodeHandle MeshUtils::CreateProceduralQuad(imp::float2 size, imp::float2 center)
{
	NodeHandle shape = GetNode();

	imp::TextureFactory::Options options;
	options.min_filter = filament::backend::SamplerMinFilter::NEAREST_MIPMAP_LINEAR;
	options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
	options.wrap_mode = filament::backend::SamplerWrapMode::REPEAT;

	auto texture = GetView().GetTextureFactory().CreateTexture(*texture_asset_, options);
	auto mask = GetView().GetTextureFactory().CreateTexture(*mask_asset_);
	auto animation = GetView().GetTextureFactory().CreateTexture(*animation_asset_);
	auto material = GetView().GetMaterialFactory().CreateMaterial(material_asset_);

	MeshPtr shape_mesh = GetView().GetMeshFactory().CreateQuad({size, center});
	material_ = material.get();

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis01(0.0f, 1.0f);

	auto shape_renderer = shape->AddComponent<RenderComponent>(RenderComponent::FrustrumCullingMode::kDisabled);

	material_->SetParameter("Noise", std::move(texture));
	material_->SetParameter("Mask", std::move(mask));
	material_->SetParameter("Animation", std::move(animation));

	shape_renderer->SetMesh(std::move(shape_mesh));
	shape_renderer->SetMaterial(material_);

	return shape;
}