#include <random>
#include <utility>

#include "light_ray.h"
#include "imp.h"
#include "core/common/log.h"
#include "native/components/sunshine_vfx/sunshine_vfx_assets.h"

using namespace imp;
using namespace std;

namespace ix::samsung::homecomponents
{
    void LightRay::Setup(){
    }

    void LightRay::Setup(string name,
                         imp::AssetPtr<imp::ImageAsset> texture_asset,
                         imp::AssetPtr<imp::MaterialAsset> material_asset,
                         imp::float2 alpha_range,
                         float fade_speed,
                         imp::float3 color)
    {
        name_ = std::move(name);
        texture_asset_ = texture_asset;
        material_asset_ = material_asset;
        color_ = color;
        alpha_range_ = alpha_range;
        fade_speed_ = fade_speed;
    }

    NodeHandle LightRay::CreateProceduralQuad(imp::float2 size, imp::float2 center)
    {
        NodeHandle shape = GetNode();

        auto material = GetView().GetMaterialFactory().CreateMaterial(material_asset_);
        auto texture = GetView().GetTextureFactory().CreateTexture(*texture_asset_);
        MeshPtr shape_mesh = GetView().GetMeshFactory().CreateQuad({size, center});
        material_ = material.get();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis01(0.0f, 1.0f);

        auto shape_renderer = shape->AddComponent<RenderComponent>(RenderComponent::FrustrumCullingMode::kDisabled);

        material_->SetParameter("texture", std::move(texture));
        material_->SetParameter("alphaRange", alpha_range_);
        material_->SetParameter("fadeSpeed", fade_speed_);
        material_->SetParameter("color", color_);
        material_->SetParameter("fadeOffset", dis01(gen) > 0.5f ? 0.0f : 3.1415f);

        shape_renderer->SetMesh(std::move(shape_mesh));
        shape_renderer->SetMaterial(std::move(material));

        return shape;
    }

    void LightRay::SetColor(imp::float3 color)
    {
        if(material_ != nullptr)
        {
            material_->SetParameter("color", color);
        }
    }

    void LightRay::SetPlaneFadeSpeed(float fade_speed)
    {
        if(material_ != nullptr)
        {
            material_->SetParameter("fadeSpeed", fade_speed);
        }
    }

    NodeHandle LightRay::CreateFromGltfAsset(AssetPtr<GltfAsset> gltf)
    {
        auto material = GetView().GetMaterialFactory().CreateMaterial(material_asset_);
        auto texture = GetView().GetTextureFactory().CreateTexture(*texture_asset_);
        NodeHandle gltf_node = GetNode();
        auto shape_renderer = gltf_node->AddComponent<GltfRenderer>(gltf);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis01(0.0f, 1.0f);

        material_ = material.get();
        material_->SetParameter("texture", std::move(texture));
        material_->SetParameter("alphaRange", alpha_range_);
        material_->SetParameter("fadeSpeed", fade_speed_);
        material_->SetParameter("color", color_);
        material_->SetParameter("fadeOffset", dis01(gen) > 0.5f ? 0.0f : 3.1415f);

        shape_renderer->SetMaterialOverrideByIndex(std::move(material), 0);

        return gltf_node;
    }
}