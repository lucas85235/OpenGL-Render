#include "imp.h"
#include "native/components/sand_wind/sand_wind.h"
#include "native/components/sand_wind/sand_wind_assets.h"
#include "native/components/particle_system/particle_system.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void SandWind::Setup()
    {
        // Particles
        state_.particlesColor = state_.particlesColor.value_or(float4(1.0, 0.819, 0.446, 1.0 ));
        state_.particleLife = state_.particleLife.value_or(10.0);
        state_.emissionRate = state_.emissionRate.value_or(0.47);
        state_.initialScale = state_.initialScale.value_or(0.6);
        state_.finalScale = state_.finalScale.value_or(1.6);
        state_.emitterWidth = state_.emitterWidth.value_or(50);
        state_.particleMinVelocity = state_.particleMinVelocity.value_or(-0.3);
        state_.particleMaxVelocity = state_.particleMaxVelocity.value_or(-0.5);
        state_.particlesOffset = state_.particlesOffset.value_or(float3(25.0, -11.0, -2.5 ));
        state_.hideParticles = state_.hideParticles.value_or(false);
        state_.particleMinVerticalVelocity = state_.particleMinVerticalVelocity.value_or(0.055);
        state_.particleMaxVerticalVelocity = state_.particleMaxVerticalVelocity.value_or(0.075);

        // Fade cycle
        state_.fadingTime = state_.fadingTime.value_or(3);
        state_.runningTime = state_.runningTime.value_or(10.0);
        state_.stoppedTime = state_.stoppedTime.value_or(10.0);

        InitializeParticles();
        InitializeFadeCycle();
    }

    void SandWind::InitializeNoise()
    {
        auto material_future = GetView().GetAssetManager().LoadMaterial(assets::kSandWindLayerMaterialCmat);
        auto texture_future = GetView().GetAssetManager().LoadImage(assets::kTXSandWindSeamlessPng);
        auto mask_future = GetView().GetAssetManager().LoadImage(assets::kTXNoiseMask2Png);
        auto noise1_future = GetView().GetAssetManager().LoadImage(assets::kTXNoiseShape1Png);
        auto noise2_future = GetView().GetAssetManager().LoadImage(assets::kTXNoiseShape2Png);

        material_future.Merge(texture_future, mask_future, noise1_future, noise2_future)
            .Then([this](std::tuple<AssetPtr<MaterialAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>> result) {
                auto [material, texture, alpha_mask, noise1, noise2] = result;

                TextureFactory::Options options;
                options.min_filter = filament::backend::SamplerMinFilter::NEAREST_MIPMAP_LINEAR;
                options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
                options.wrap_mode = filament::backend::SamplerWrapMode::REPEAT;

                CreateQuadSettings settings;
                settings.size = state_.layerSize.value();

                auto panelMesh = GetView().GetMeshFactory().CreateQuad(settings);
                auto node = GetView().CreateNode();
                node->SetName("SandWindLayer");
                node->SetParent(GetNode());
                node->SetLocalPosition(state_.layerOffset.value());

                ComponentHandle<RenderComponent> renderer_model = node->AddComponent<RenderComponent>(RenderComponent::FrustrumCullingMode::kDisabled);
                renderer_model->SetMesh(std::move(panelMesh));
                renderer_model->SetPriority(5);

                auto mat = GetView().GetMaterialFactory().CreateMaterial(material);
                layer_material_ptr_ = mat.get();
                layer_material_ptr_->SetParameter("Texture", GetView().GetTextureFactory().CreateTexture(*texture, options));
                layer_material_ptr_->SetParameter("AlphaMask", GetView().GetTextureFactory().CreateTexture(*alpha_mask, options));
                layer_material_ptr_->SetParameter("Noise1", GetView().GetTextureFactory().CreateTexture(*noise1, options));
                layer_material_ptr_->SetParameter("Noise2", GetView().GetTextureFactory().CreateTexture(*noise2, options));
                layer_material_ptr_->SetParameter("NoiseStrength", state_.noiseStrength.value());
                layer_material_ptr_->SetParameter("Fade", 0);

                renderer_model->SetMaterial(std::move(mat));

                OnIsfStateChanged();

                layer_node_ = node;
                layer_node_->SetEnabled(!state_.hideLayer.value());

                state_machine_.material_ = layer_material_ptr_;
            }).KeptBy(this);
    }

    void SandWind::Update(const imp::FrameTime &frame_time)
    {
        state_machine_.Update(frame_time.GetDeltaSeconds());
    }

    void SandWind::InitializeParticles()
    {
        auto particle_material_future = GetView().GetAssetManager().LoadMaterial(assets::kSandWindParticlesMaterialCmat);
        auto particle_texture_future = GetView().GetAssetManager().LoadImage(assets::kTXSandWindCirclePng);
        auto mask_future = GetView().GetAssetManager().LoadImage(assets::kTXParticlesMaskPng);

        particle_material_future.Merge(particle_texture_future, mask_future).Then(
            [this](std::tuple<AssetPtr<MaterialAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset> > result)
            {
            auto [material, image, mask] = result;

            auto particle_node = GetView().CreateNode();
            particle_node->SetName("SandWindParticles");
            particle_node->SetParent(GetNode());
            particle_node->SetLocalPosition(state_.particlesOffset.value());

            TextureFactory::Options texture_options;

            // Particle material
            MaterialPtr ps_material = GetView().GetMaterialFactory().CreateMaterial(material);
            auto ps_texture = GetView().GetTextureFactory().CreateTexture(*image);
            ps_material->SetParameter("BaseTexture", std::move(ps_texture));
            ps_material->SetParameter("BaseColor", filament::RgbaType::LINEAR, state_.particlesColor.value());
            ps_material->SetParameter("Mask", GetView().GetTextureFactory().CreateTexture(*mask, texture_options));
            ps_material->SetParameter("InitialScale", state_.initialScale.value());
            ps_material->SetParameter("FinalScale", state_.finalScale.value());

            particle_material_ptr_ = ps_material.get();

            // Create and setup particle system
            auto options = ParticleSystemOptions();

            // Main options
            options.max_particles = 150;
            options.looping = false;

            // Renderer options
            options.mesh_type = ParticleMesh::QUAD;
            options.limit_update = true;
            options.render_priority = 2;

            // Emission options
            options.emission_enabled = true;

            auto emitterOptions = EmitterOptions();
            emitterOptions.particle_life = state_.particleLife.value();
            emitterOptions.emission_rate = state_.emissionRate.value();
            emitterOptions.emission_quantity = 1;
            emitterOptions.is_emitter = true;
            float half_emitter_width = state_.emitterWidth.value() / 2.0f;
            emitterOptions.positionFunction = [=](NodeHandle node)
            {
                return RandomValue(float3{ -half_emitter_width, 0.0, -4.0 },
                                   float3{ half_emitter_width, 1.0, 4.0 }).getValue();
            };

            emitterOptions.velocityFunction = [=]()
            {
                return RandomValue(float3 { state_.particleMinVelocity.value(), state_.particleMinVerticalVelocity.value(), 0.0 },
                                   float3 { state_.particleMaxVelocity.value(), state_.particleMaxVerticalVelocity.value(), 0.0}).getValue();
            };

            emitterOptions.scaleFunction = [](){return float3(12.0);};

            auto particle_system = particle_node->AddComponent<ParticleSystem>(options);
            particle_system->AddEmitter(emitterOptions, float3(0), QuatFromEuler(float3(0)));
            particle_system->Initialize(std::move(ps_material));
            particle_system->Play();

            OnIsfStateChanged();

            particle_node_ = particle_node;
            particle_node_->SetEnabled(!state_.hideParticles.value());

            state_machine_.particle_system_ = particle_system;
        }).KeptBy(this);
    }

    void SandWind::InitializeDust()
    {
        auto particle_material_future = GetView().GetAssetManager().LoadMaterial(assets::kSandWindParticlesMaterialCmat);
        auto particle_texture_future = GetView().GetAssetManager().LoadImage(assets::kTXSandWindCirclePng);
        auto mask_future = GetView().GetAssetManager().LoadImage(assets::kTXParticlesMaskPng);

        particle_material_future.Merge(particle_texture_future, mask_future).Then(
            [this](std::tuple<AssetPtr<MaterialAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset> > result)
            {
            auto [material, image, mask] = result;

            auto dust_node = GetView().CreateNode();
            dust_node->SetName("SandWindDust");
            dust_node->SetParent(GetNode());

            dust_node->SetLocalPosition(state_.dustParticlesOffset.value());

            // Particle material
            MaterialPtr ps_material = GetView().GetMaterialFactory().CreateMaterial(material);
            auto ps_texture = GetView().GetTextureFactory().CreateTexture(*image);
            ps_material->SetParameter("BaseTexture", std::move(ps_texture));
            ps_material->SetParameter("BaseColor", filament::RgbaType::PREMULTIPLIED_LINEAR, state_.dustParticlesColor.value());
            ps_material->SetParameter("Mask", GetView().GetTextureFactory().CreateTexture(*mask));
            ps_material->SetParameter("InitialScale", state_.dustInitialScale.value());
            ps_material->SetParameter("FinalScale", state_.dustFinalScale.value());

            dust_particle_material_ptr_ = ps_material.get();

            // Create and setup particle system
            auto options = ParticleSystemOptions();

            // Main options
            options.max_particles = 350;
            options.looping = false;

            // Renderer options
            options.mesh_type = ParticleMesh::QUAD;
            options.limit_update = true;
            options.render_priority = 3;

            // Emission options
            options.emission_enabled = true;

            auto emitterOptions = EmitterOptions();
            emitterOptions.particle_life = state_.dustParticleLife.value();
            emitterOptions.emission_rate = state_.dustEmissionRate.value();
            emitterOptions.is_emitter = true;
            emitterOptions.emission_quantity = 1;
            float half_emitter_width = state_.dustEmitterWidth.value() / 2.0f;
            emitterOptions.positionFunction = [=](NodeHandle node)
            {
                return RandomValue(float3{ -half_emitter_width, -0.2, -4.0 },
                                   float3{ half_emitter_width, 0.2, 4.0 }).getValue();
            };

            emitterOptions.velocityFunction = [=]()
            {
                return RandomValue(float3 { state_.dustParticleMinVelocity.value(), 0.05, 0.0 },
                                   float3 { state_.dustParticleMaxVelocity.value(), 0.1, 0.0}).getValue();
            };

            emitterOptions.scaleFunction = [](){return float3(12.0);};

            auto dust_particle_system = dust_node->AddComponent<ParticleSystem>(options);
            dust_particle_system->AddEmitter(emitterOptions, float3(0), QuatFromEuler(float3(0)));
            dust_particle_system->Initialize(std::move(ps_material));
            dust_particle_system->Play();

            OnIsfStateChanged();

            dust_node_ = dust_node;

            state_machine_.dust_particle_system_ = dust_particle_system;
        }).KeptBy(this);
    }

    void SandWind::InitializeFadeCycle()
    {
        state_machine_.running_time_ = state_.runningTime.value();
        state_machine_.stopped_time_ = state_.stoppedTime.value();
        state_machine_.fading_time_ = state_.fadingTime.value();
    }

    void SandWind::OnIsfStateChanged()
    {
        if(layer_material_ptr_ == nullptr) return;

        layer_material_ptr_->SetParameter("Color", filament::RgbaType::sRGB, state_.layerColor.value());
        layer_material_ptr_->SetParameter("Brightness", state_.layerBrightness.value());
        layer_material_ptr_->SetParameter("Speed", state_.layerSpeed.value());
        layer_material_ptr_->SetParameter("NoiseStrength", state_.noiseStrength.value());
    }

#if IMP_RUNTIME(DEV)
    // Customize Editor UI for this component
    void SandWind::DrawEditorUi()
    {
        if (ImGui::Button("Reload effect"))
        {
            state_machine_.Reset();
            InitializeFadeCycle();

            GetView().DestroyNode(particle_node_);
            if (!state_.hideParticles.value())
            {
                InitializeParticles();
            }

            // GetView().DestroyNode(dust_node_);
            // if (!state_.hideDustParticles.value())
            // {
            //     InitializeDust();
            // }
            //
            // layer_node_->SetEnabled(!state_.hideLayer.value());
        }
        if (ImGui::Button("Pause particle emission"))
        {
            if (!particle_node_) return;
            particle_node_->GetComponent<ParticleSystem>()->PauseEmission();
        }
        if (ImGui::Button("Start particle emission"))
        {
            if (!particle_node_) return;
            particle_node_->GetComponent<ParticleSystem>()->StartEmission();
        }
    }
#endif // IMP_RUNTIME(DEV)
}
