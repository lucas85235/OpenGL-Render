#include "imp.h"
#include "core/common/log.h"
#include "native/components/side_clouds/side_clouds_component.h"
#include "native/components/side_clouds/side_clouds_assets.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void SideCloudsComponent::Setup(SideCloudParams& params) {
        state_.cloud_density = params.kDensity;
        state_.cloud_velocity = params.kVelocity;
        state_.cloud_samples = params.kSamples;
        state_.cloud_color = params.kColor;
        Setup();
    }

    void SideCloudsComponent::Setup()
    {
        InitializeSideClouds();
    }

    void SideCloudsComponent::InitializeSideClouds()
    {
        auto clouds_mat_future = GetView().GetAssetManager().LoadMaterial(assets::kCloudsMaterialCmat);
        auto clouds_texture_future = GetView().GetAssetManager().LoadImage(assets::kTXNoisePng);

        clouds_mat_future.Merge(clouds_texture_future)
            .Then([this](std::tuple<AssetPtr<MaterialAsset>, AssetPtr<ImageAsset>> result)
            {
                auto [clouds_mat, clouds_texture] = result;

                auto node = GetView().CreateNode();
                node->SetName("Clouds Model");
                node->SetParent(GetNode());
                node->SetLocalScale(imp::float3(72.f, 50.f, 72.f));
                node->SetLocalPosition(imp::float3(0,-5.5, 30));
                node->SetLocalRotation(QuatFromEuler(float3(180,0, 0)));             

                ComponentHandle<RenderComponent> renderer_model = node->AddComponent<RenderComponent>(RenderComponent::FrustrumCullingMode::kDisabled);

                // curve plane 
                CreateQuadSettings settings;
                settings.resolution = 12;
                settings.radius = 0.45;
                settings.center = {0, 0.7};
                renderer_model->SetMesh(std::move(GetView().GetMeshFactory().CreatePanel(settings)));

                imp::TextureFactory::Options options;
                options.min_filter = filament::backend::SamplerMinFilter::LINEAR;
                options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
                options.wrap_mode = filament::backend::SamplerWrapMode::REPEAT;
                options.anisotropy = 0.0f;

                auto clouds_ptr = GetView().GetTextureFactory().CreateTexture(*clouds_texture, options);
                material_ptr_ = GetView().GetMaterialFactory().CreateMaterial(clouds_mat);

                material_ptr_->SetName("MT_CloudDome");
                material_ptr_->SetParameter("Noise", std::move(clouds_ptr));
                renderer_model->SetMaterial(material_ptr_.get());

                OnIsfStateChanged();
            }).KeptBy(this);
    }

    void SideCloudsComponent::OnIsfStateChanged() {
        if (material_ptr_ == nullptr) {
            imp::output::Info("Null Reference!");
            return;
        }

        material_ptr_->SetParameter("Density", state_.cloud_density);
        material_ptr_->SetParameter("Velocity", state_.cloud_velocity);
        material_ptr_->SetParameter("Samples", (int)state_.cloud_samples);
        material_ptr_->SetParameter("Color", state_.cloud_color);

        imp::output::Info("OnIsfStateChanged");
    }
}
