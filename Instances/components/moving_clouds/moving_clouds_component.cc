#include "imp.h"
#include "core/common/log.h"
#include "native/components/moving_clouds/moving_clouds_component.h"
#include "native/components/moving_clouds/moving_clouds_assets.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void MovingCloudsComponent::Setup(MovingCloudParams& params) {
        state_.cloud_density = params.kDensity;
        state_.cloud_velocity = params.kVelocity;
        state_.cloud_samples = params.kSamples;
        state_.clouds_small_speed = params.kSmallCloudsSpeed;
        state_.cloud_color = params.kColor;
		state_.tiling = params.tiling;
		Setup();
    }

    void MovingCloudsComponent::Setup()
    {
        InitializeClouds();
    }

    void MovingCloudsComponent::InitializeClouds()
    {        
        auto clouds_mat_future = GetView().GetAssetManager().LoadMaterial(assets::kCloudsMaterialCmat);
        auto clouds_texture_future = GetView().GetAssetManager().LoadImage(assets::kTXNoisePng);
        auto mask_texture_future = GetView().GetAssetManager().LoadImage(assets::kTXCloudsMaskPng);
        auto animation_texture_future = GetView().GetAssetManager().LoadImage(assets::kTXAnimationMaskPng);
        auto model_future = GetView().GetAssetManager().LoadModel(assets::kSMCloudsGlb);

        clouds_mat_future.Merge(clouds_texture_future, mask_texture_future, animation_texture_future, model_future)
            .Then([this](std::tuple<AssetPtr<MaterialAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>, NodeHandle> result)
            {
                auto [clouds_mat, clouds_texture, mask_texture, animation_texture, model] = result;

                auto mesh = GetView().GetMeshFactory().CreateQuad({
                    .size = imp::float2(1.0f, 1.0f)
                });

                model->SetParent(GetNode());
                model->SetName("Clouds Model");
                auto model_render = model->GetComponent<GltfRenderer>();

                imp::TextureFactory::Options options;
                options.min_filter = filament::backend::SamplerMinFilter::NEAREST_MIPMAP_LINEAR;
                options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
                options.wrap_mode = filament::backend::SamplerWrapMode::REPEAT;

                auto clouds_ptr = GetView().GetTextureFactory().CreateTexture(*clouds_texture, options);
                auto mask_ptr = GetView().GetTextureFactory().CreateTexture(*mask_texture);
                auto animation_ptr = GetView().GetTextureFactory().CreateTexture(*animation_texture);
                material_ptr_ = GetView().GetMaterialFactory().CreateMaterial(clouds_mat);

                material_ptr_->SetName("MT_CloudDome");
                material_ptr_->SetParameter("Noise", std::move(clouds_ptr));
                material_ptr_->SetParameter("Mask", std::move(mask_ptr));
                material_ptr_->SetParameter("Animation", std::move(animation_ptr));
				material_ptr_->SetParameter("Tiling", state_.tiling);

				model_render->SetMaterialOverrideByIndex(material_ptr_.get(), 0);

				model->SetEnabled(false);

				CreateQuads(imp::float2{0.0,0.0});

				VisitNodeTreeToDisableGltfColliders(model);
                OnIsfStateChanged();
            }).KeptBy(this);        
    }

    void MovingCloudsComponent::OnIsfStateChanged() {
        if (material_ptr_ == nullptr) {
            imp::output::Info("Null Reference!");
            return;
        }

        material_ptr_->SetParameter("Density", state_.cloud_density);
        material_ptr_->SetParameter("Velocity", state_.cloud_velocity);
        material_ptr_->SetParameter("SmallCloudsSpeed", state_.clouds_small_speed);
        material_ptr_->SetParameter("Samples", (int)state_.cloud_samples);
        material_ptr_->SetParameter("Color", state_.cloud_color);
		material_ptr_->SetParameter("Tiling", state_.tiling);

		imp::output::Info("OnIsfStateChanged");
    }

    void MovingCloudsComponent::VisitNodeTreeToDisableGltfColliders(NodeHandle node){
        if(node->GetComponent<GltfCollider>().IsValid()){
            node->GetComponent<GltfCollider>()->SetEnabled(false);
        }
        for(auto node : node->GetChildren()){
            VisitNodeTreeToDisableGltfColliders(node);
        }
    }

	void MovingCloudsComponent::CreateQuads(float2 pos)
	{
		MeshPtr shape_mesh = GetView().GetMeshFactory().CreateQuad({imp::float2{1.0,1.0}, imp::float2{.0,.0} });
		imp::NodeHandle quad = GetView().CreateNode();

		auto quadrenderer = quad->AddComponent<RenderComponent>(RenderComponent::FrustrumCullingMode::kDisabled);

		quad->SetLocalPosition(imp::float3{pos.x,10.0,pos.y});
		quad->SetLocalScale(imp::float3 {50.0,50.0,50.0});
		quad->SetLocalRotation(imp::QuatFromEuler(imp::float3{90.0,0.0,0.0}));
		quad->SetName("ProceduralQuad");
        quad->SetParentKeepWorldTransform(GetNode());

		material_ptr_->SetParameter("Tiling", 1.3f);

		quadrenderer->SetMesh(std::move(shape_mesh));
		quadrenderer->SetMaterial(material_ptr_.get());
	}
}
