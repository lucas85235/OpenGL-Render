#include <tuple>

#include "imp.h"
#include "native/components/grass/grass.h"
#include "native/components/grass/grass_position.h"
#include "native/components/grass/grass_assets.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void Grass::Setup()
    {
        InitializeParticles();
        InstanceParticles();
    }

    void Grass::InitializeParticles()
    {        
        state_.grassAmount = state_.grassAmount.value_or(grass_data_.amount);
        state_.grassColor1 = state_.grassColor1.value_or(grass_data_.color1);
        state_.grassColor2 = state_.grassColor2.value_or(grass_data_.color2);

        state_.windFrequency = state_.windFrequency.value_or(grass_data_.wind_frequency);
        state_.waveSize = state_.waveSize.value_or(grass_data_.wave_size);

        state_.lodMinDistance = state_.lodMinDistance.value_or(grass_data_.lod_min_distance);
        state_.lodMaxDistance = state_.lodMaxDistance.value_or(grass_data_.lod_max_distance);

        state_.info->position = state_.info->position.value_or(float3(0.0));
        state_.info->scale = state_.info->scale.value_or(float3(1.0));
        state_.info->rotation = state_.info->rotation.value_or(float3(0.0));
    }

    void Grass::InstanceParticles()
    {
        auto material_future = GetView().GetAssetManager().LoadMaterial(assets::kGrassMaterialCmat);
        auto grass_future1 = GetView().GetAssetManager().LoadImage(assets::kTXGrass01Png);
        auto grass_future2 = GetView().GetAssetManager().LoadImage(assets::kTXGrass02Png);
        auto mask_future = GetView().GetAssetManager().LoadImage(assets::kTXGrassInfluencePng);
        material_future.Merge(grass_future1, grass_future2, mask_future).Then([=](std::tuple<AssetPtr<MaterialAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>> result) {
            auto [material, grass1, grass2, mask] = result;

            auto particle_node = GetView().CreateNode();
            particle_node->SetName("GrassParticles");
            particle_node->SetParent(GetNode());

            auto params = ParticleInstancesParams();
            auto doubleQuad = MeshQuad();
            auto domeShape = RandomPosition();

            imp::TextureFactory::Options options;
            options.min_filter = filament::backend::SamplerMinFilter::LINEAR_MIPMAP_LINEAR;
            options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
            options.wrap_mode = filament::backend::SamplerWrapMode::REPEAT;
            options.generated_mipmap_levels = 32.0;

            params.info.position = state_.info->position.value();
            params.info.scale = state_.info->scale.value();
            params.info.rotation = state_.info->rotation.value();

            params.amount = state_.grassAmount.value();
            params.angle = true;
            params.mesh = &doubleQuad;
            params.texture = GetView().GetTextureFactory().CreateTexture(*grass1, options);

            params.positionFunction = domeShape.get();
            params.material = GetView().GetMaterialFactory().CreateMaterial(material);
            params.material->SetParameter("Grass2", GetView().GetTextureFactory().CreateTexture(*grass2, options));
            params.material->SetParameter("Mask", GetView().GetTextureFactory().CreateTexture(*mask));
            params.material->SetParameter("GrassColor1",      grass_data_.color1);
            params.material->SetParameter("GrassColor2",      grass_data_.color2);
            params.material->SetParameter("WindFrequency",   grass_data_.wind_frequency);
            params.material->SetParameter("WaveSize",        grass_data_.wave_size);
            params.material->SetParameter("LodMinDistance",  grass_data_.lod_min_distance);
            params.material->SetParameter("LodMaxDistance",  grass_data_.lod_max_distance);

            particle_material_ptr_ = params.material.get();
            particle_node_ = particle_node;
            particle_node->AddComponent<ParticleInstances>(params);
        }).KeptBy(this);
    }

    void Grass::OnIsfStateChanged()
    {
        grass_data_.color1 = state_.grassColor1.value();
        grass_data_.color2 = state_.grassColor2.value();
        grass_data_.amount = state_.grassAmount.value();

        if(particle_material_ptr_ == nullptr) return;
        particle_material_ptr_->SetParameter("GrassColor1",      state_.grassColor1.value());
        particle_material_ptr_->SetParameter("GrassColor2",      state_.grassColor2.value());
        particle_material_ptr_->SetParameter("WindFrequency",   state_.windFrequency.value());
        particle_material_ptr_->SetParameter("WaveSize",        state_.waveSize.value());
        particle_material_ptr_->SetParameter("LodMinDistance",  state_.lodMinDistance.value());
        particle_material_ptr_->SetParameter("LodMaxDistance",  state_.lodMaxDistance.value());
    }

    void Grass::ResetEffect()
    {
        GetView().DestroyNode(particle_node_);
        InstanceParticles();
    }

#if IMP_RUNTIME(DEV)
    void Grass::DrawEditorUi()
    {
        if (ImGui::Button("Reload effect"))
        {
            ResetEffect();
        }
    }
#endif // IMP_RUNTIME(DEV)

}  // namespace ix::samsung::homecomponents
