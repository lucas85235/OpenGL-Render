#include "imp.h"
#include "native/components/trail/trail.h"

using namespace imp;

namespace ix::samsung::homecomponents {

    void Trail::Setup() {
        InitializeTrail();
    }

    void Trail::InitializeTrail() {
        particle_instance_node_ = GetView().CreateNode();
        particle_instance_node_->SetName("Particle Node");
        particle_instance_node_->SetParentKeepWorldTransform(GetNode());

        trailData_.play = state_.play;
        InstanceTrail(trailData_);
    }

    void Trail::InstanceTrail(TrailData td_) {
        auto material = GetView().GetAssetManager().LoadMaterial(assets::kTrailMaterialCmat);
        auto texture = GetView().GetAssetManager().LoadImage(assets::kTexturePng);

        material.Merge(texture).Then([=](std::tuple<imp::AssetPtr<imp::MaterialAsset>, imp::AssetPtr<imp::ImageAsset>> result) {
            auto [material, texture] = result;
            auto params = ParticleInstancesParams();

            particle_instance_child_node_ = GetView().CreateNode();
            particle_instance_child_node_->SetParentKeepWorldTransform(particle_instance_node_);

            imp::TextureFactory::Options options;
            options.min_filter = filament::backend::SamplerMinFilter::LINEAR_MIPMAP_LINEAR;
            options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
            options.wrap_mode = filament::backend::SamplerWrapMode::REPEAT;
            params.texture = GetView().GetTextureFactory().CreateTexture(*texture, options);

            MeshQuad mesh;
            params.mesh = &mesh;
            params.amount = 1000;
            params.radius = 7.0;
            LinePositionSequential domeShape = LinePositionSequential();
            params.positionFunction = domeShape.get();
            params.info.position = float3(0.0, 0.0, -20.0);

            trail_material = GetView().GetMaterialFactory().CreateMaterial(material);
            trail_material->SetParameter("Play", td_.play);

            params.material = std::move(trail_material);
            tr_material_ = params.material.get();
            particle_instance_node_->AddComponent<ParticleInstances>(params);

        }).KeptBy(this);
    }

    void Trail::Update(const FrameTime& frame_time)
    {
        if (tr_material_) {
            tr_material_->SetParameter("Play", state_.play);
        }
    }
}