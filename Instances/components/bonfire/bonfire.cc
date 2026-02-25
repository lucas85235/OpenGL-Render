#include "imp.h"
#include "native/components/bonfire/bonfire.h"
#include "native/components/bonfire/bonfire_assets.h"
#include "native/components/particle_system/particle_system.h"

#if IMP_RUNTIME(DEV)
#include "dear_imgui/imgui.h"
#endif

using namespace imp;

namespace ix::samsung::homecomponents
{
    void Bonfire::Setup() {
        // Primary flame (external)
        state_.primaryFlame.color1 = state_.primaryFlame.color1.value_or(float4(0.991, 0.425, 0.021, 0.589));
        state_.primaryFlame.color2 = state_.primaryFlame.color2.value_or(float4(0.996, 0.830, 0.013, 0.710));
        state_.primaryFlame.smokeColor = state_.primaryFlame.smokeColor.value_or(float4(0.606, 0.485, 0.354, 0.628));
        state_.primaryFlame.bloomColor = state_.primaryFlame.bloomColor.value_or(float4(0.922, 0.744, 0.479, 0.048));
        state_.primaryFlame.bloomSize = state_.primaryFlame.bloomSize.value_or(1.32);
        state_.primaryFlame.bloomOffsetX = state_.primaryFlame.bloomOffsetX.value_or(0.16);
        state_.primaryFlame.bloomOffsetY = state_.primaryFlame.bloomOffsetY.value_or(0.33);
        state_.primaryFlame.offsetX = state_.primaryFlame.offsetX.value_or(-0.10);
        state_.primaryFlame.offsetY = state_.primaryFlame.offsetY.value_or(0.11);
        state_.primaryFlame.noiseStrength = state_.primaryFlame.noiseStrength.value_or(0.18);
        state_.primaryFlame.brightness = state_.primaryFlame.brightness.value_or(3.47);
        state_.primaryFlame.speed = state_.primaryFlame.speed.value_or(0.83);
        state_.primaryFlame.size = state_.primaryFlame.size.value_or(float2(8.0, 10.0));
        state_.primaryFlame.smokeHeight = state_.primaryFlame.smokeHeight.value_or(0.60);

        // Secondary flame (internal)
        state_.secondaryFlame.color1 = state_.secondaryFlame.color1.value_or(float4(1.0, 0.338, 0.0, 0.628));
        state_.secondaryFlame.color2 = state_.secondaryFlame.color2.value_or(float4(1.0, 0.338, 0.0, 0.740));
        state_.secondaryFlame.smokeColor = state_.secondaryFlame.smokeColor.value_or(float4(0.3, 0.3, 0.3, 1.0));
        state_.secondaryFlame.bloomColor = state_.secondaryFlame.bloomColor.value_or(float4(0.0));
        state_.secondaryFlame.bloomSize = state_.secondaryFlame.bloomSize.value_or(1.0);
        state_.secondaryFlame.bloomOffsetX = state_.secondaryFlame.bloomOffsetX.value_or(0.0);
        state_.secondaryFlame.bloomOffsetY = state_.secondaryFlame.bloomOffsetY.value_or(0.0);
        state_.secondaryFlame.offsetX = state_.secondaryFlame.offsetX.value_or(-0.10);
        state_.secondaryFlame.offsetY = state_.secondaryFlame.offsetY.value_or(0);
        state_.secondaryFlame.noiseStrength = state_.secondaryFlame.noiseStrength.value_or(0.19);
        state_.secondaryFlame.brightness = state_.secondaryFlame.brightness.value_or(3.62);
        state_.secondaryFlame.speed = state_.secondaryFlame.speed.value_or(1.36);
        state_.secondaryFlame.size = state_.secondaryFlame.size.value_or(float2(4.0, 4.0));
        state_.secondaryFlame.smokeHeight = state_.secondaryFlame.smokeHeight.value_or(1.0);

        // Base flame (at the bottom of the bonfire)
        state_.baseFlame.color1 = state_.baseFlame.color1.value_or(float4(1.0, 0.208, 0, 0.675));
        state_.baseFlame.color2 = state_.baseFlame.color2.value_or(float4(0.920, 0.948, 0.513, 0.939));
        state_.baseFlame.speed = state_.baseFlame.speed.value_or(0.6);
        state_.baseFlame.size = state_.baseFlame.size.value_or(float2(8.0, 8.0));

        // Sparks
        state_.sparks.color = state_.sparks.color.value_or(float4(0.926, 0.570, 0.257, 0.375));
        state_.sparks.offset = state_.sparks.offset.value_or(float3(0, -2.0, -1.0));
        state_.sparks.particleLife = state_.sparks.particleLife.value_or(2.49);
        state_.sparks.emissionRate = state_.sparks.emissionRate.value_or(0.185);
        state_.sparks.emitterWidth = state_.sparks.emitterWidth.value_or(2.5f);
        state_.sparks.minVelocity = state_.sparks.minVelocity.value_or(13.6);
        state_.sparks.maxVelocity = state_.sparks.maxVelocity.value_or(38.3);
        state_.sparks.emissionConeAngle = state_.sparks.emissionConeAngle.value_or(15);
        state_.sparks.particleScale = state_.sparks.particleScale.value_or(0.3);

        // Wind
        state_.wind.speed = state_.wind.speed.value_or(2.0);
        state_.wind.amplitude = state_.wind.amplitude.value_or(18.0);

        // Ground light
        state_.groundLight.color = state_.groundLight.color.value_or(float4(0.944, 0.718, 0.249, 0.273));
        state_.groundLight.offset = state_.groundLight.offset.value_or(float3(0.0, -3.0, 1.3));
        state_.groundLight.frequency = state_.groundLight.frequency.value_or(1.0);
        state_.groundLight.alphaLowerMultiplier = state_.groundLight.alphaLowerMultiplier.value_or(0.8);
        state_.groundLight.size = state_.groundLight.size.value_or(float2(20.0, 20.0));
        Initialize();
    }

    void Bonfire::Initialize()
    {
        AddBonfireSprite();
        AddGroundLightSprite();
        AddFlames();
        AddBase({0, -0.3, 0.4}, state_.baseFlame);
        AddSparks(state_.sparks);
    }

    void Bonfire::AddFlames() {
        flames_node_ = GetView().CreateNode();
        flames_node_->SetParent(GetNode());
        flames_node_->SetName("MainFlames");

        AddFlame("PrimaryFlame", {0.0, 0.0, 0}, state_.primaryFlame, 2);
        AddFlame("SecondaryFlame", {0.0, -1.5, 0.2}, state_.secondaryFlame, 3);
    }

    void Bonfire::AddFlame(const std::string& node_name, float3 local_pos, FlameState state, int priority)
    {
        auto material_future = GetView().GetAssetManager().LoadMaterial(assets::kFlameMaterialCmat);
        auto texture_future = GetView().GetAssetManager().LoadImage(assets::kTXFlameShapeSmooth1Png);
        auto noise1_future = GetView().GetAssetManager().LoadImage(assets::kTXFlameNoise5Jpg);
        auto noise2_future = GetView().GetAssetManager().LoadImage(assets::kTXFlameNoise2Png);

        material_future.Merge(texture_future, noise1_future, noise2_future)
            .Then([this, node_name, local_pos, state, priority](std::tuple<AssetPtr<MaterialAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>> result) {
                auto [material, texture, noise1, noise2] = std::move(result);

                TextureFactory::Options options;
                options.min_filter = filament::backend::SamplerMinFilter::NEAREST_MIPMAP_LINEAR;
                options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
                options.wrap_mode = filament::backend::SamplerWrapMode::REPEAT;

                CreateQuadSettings settings;
                settings.size = state.size.value();
                settings.resolution = 8;

                auto panelMesh = GetView().GetMeshFactory().CreatePanel(settings);
                auto node = GetView().CreateNode();
                node->SetName(node_name);
                node->SetParent(flames_node_);
                node->SetLocalPosition(local_pos);

                ComponentHandle<RenderComponent> renderer_model = node->AddComponent<RenderComponent>(RenderComponent::FrustrumCullingMode::kDisabled);
                renderer_model->SetMesh(std::move(panelMesh));
                renderer_model->SetPriority(priority);

                auto mat = GetView().GetMaterialFactory().CreateMaterial(material);
                mat->SetParameter("Texture", GetView().GetTextureFactory().CreateTexture(*texture));
                mat->SetParameter("Noise1", GetView().GetTextureFactory().CreateTexture(*noise1, options));
                mat->SetParameter("Noise2", GetView().GetTextureFactory().CreateTexture(*noise2, options));
                mat->SetParameter("Color1", filament::RgbaType::sRGB, state.color1.value());
                mat->SetParameter("Color2", filament::RgbaType::sRGB, state.color2.value());
                mat->SetParameter("SmokeColor", filament::RgbaType::sRGB, state.smokeColor.value());
                mat->SetParameter("BloomColor", filament::RgbaType::sRGB, state.bloomColor.value());
                mat->SetParameter("BloomSize", state.bloomSize.value());
                mat->SetParameter("Brightness", state.brightness.value());
                mat->SetParameter("Speed", state.speed.value());
                mat->SetParameter("NoiseStrength", state.noiseStrength.value());
                mat->SetParameter("SmokeHeight", state.smokeHeight.value());
                mat->SetParameter("BloomOffset", float2(state.bloomOffsetX.value(), state.bloomOffsetY.value()));
                mat->SetParameter("Offset", float2(state.offsetX.value(), state.offsetY.value()));
                mat->SetParameter("WindSpeed", state_.wind.speed.value());
                mat->SetParameter("WindAmplitude", state_.wind.amplitude.value());
                mat->SetParameter("TimeOffset", RandomNumber(0.0, 1.0));

                renderer_model->SetMaterial(std::move(mat));
            }).KeptBy(this);
    }

    void Bonfire::AddBase(float3 local_pos, BonfireBaseState state) {
        base_node_ = GetView().CreateNode();

        auto material_future = GetView().GetAssetManager().LoadMaterial(assets::kBonfireBaseMaterialCmat);
        auto noise_future = GetView().GetAssetManager().LoadImage(assets::kTXFlameNoise2Png);
        auto gradient_future = GetView().GetAssetManager().LoadImage(assets::kTXBaseGradientPng);
        auto alpha_mask_future = GetView().GetAssetManager().LoadImage(assets::kTXBaseGlowPng);

        material_future.Merge(noise_future, gradient_future, alpha_mask_future)
            .Then([this, local_pos, state](std::tuple<AssetPtr<MaterialAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>> result) {
                auto [material, noise, gradient, alpha_mask] = std::move(result);

                TextureFactory::Options options;
                options.min_filter = filament::backend::SamplerMinFilter::NEAREST_MIPMAP_LINEAR;
                options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
                options.wrap_mode = filament::backend::SamplerWrapMode::REPEAT;

                CreateQuadSettings settings;
                settings.size = state.size.value();

                auto sprite_mesh = GetView().GetMeshFactory().CreateQuad(settings);
                base_node_->SetLocalPosition(local_pos);
                base_node_->SetName("BaseFlame");
                base_node_->SetParent(GetNode());
                auto renderer_model = base_node_->AddComponent<RenderComponent>(RenderComponent::FrustrumCullingMode::kDisabled);
                renderer_model->SetMesh(std::move(sprite_mesh));
                renderer_model->SetPriority(4);

                auto mat = GetView().GetMaterialFactory().CreateMaterial(material);
                mat->SetParameter("Noise", GetView().GetTextureFactory().CreateTexture(*noise));
                mat->SetParameter("Gradient", GetView().GetTextureFactory().CreateTexture(*gradient));
                mat->SetParameter("AlphaMask", GetView().GetTextureFactory().CreateTexture(*alpha_mask));
                mat->SetParameter("Color1", filament::RgbaType::sRGB, state.color1.value());
                mat->SetParameter("Color2", filament::RgbaType::sRGB, state.color2.value());
                mat->SetParameter("Speed", state.speed.value());
                renderer_model->SetMaterial(std::move(mat));
            }).KeptBy(this);
    }

    void Bonfire::AddBonfireSprite() {
        bonfire_sprite_node_ = GetView().CreateNode();

        auto sprite_material_future = GetView().GetAssetManager().LoadMaterial(assets::kBonfireSpriteMaterialCmat);
        auto sprite_future = GetView().GetAssetManager().LoadImage(assets::kTXBonfire1024Png);

        sprite_material_future.Merge(sprite_future)
            .Then([this](std::tuple<AssetPtr<MaterialAsset>, AssetPtr<ImageAsset>> result) {
                auto [material_asset, texture_asset] = std::move(result);

                CreateQuadSettings settings;
                settings.size = {10, 10};
                auto sprite_mesh = GetView().GetMeshFactory().CreateQuad(settings);
                bonfire_sprite_node_->SetName("BonfireSprite");
                bonfire_sprite_node_->SetParent(GetNode());
                auto renderer_model = bonfire_sprite_node_->AddComponent<RenderComponent>(RenderComponent::FrustrumCullingMode::kDisabled);
                renderer_model->SetMesh(std::move(sprite_mesh));
                renderer_model->SetPriority(1);

                auto material = GetView().GetMaterialFactory().CreateMaterial(material_asset);
                material->SetParameter("BaseTexture", GetView().GetTextureFactory().CreateTexture(*texture_asset));
                renderer_model->SetMaterial(std::move(material));
            }).KeptBy(this);
    }

    void Bonfire::AddGroundLightSprite() {
        ground_light_sprite_node_ = GetView().CreateNode();

        auto sprite_material_future = GetView().GetAssetManager().LoadMaterial(assets::kGroundLightMaterialCmat);
        auto sprite_future = GetView().GetAssetManager().LoadImage(assets::kTXLightGroundPng);

        sprite_material_future.Merge(sprite_future)
            .Then([this](std::tuple<AssetPtr<MaterialAsset>, AssetPtr<ImageAsset>> result) {
                auto [material_asset, texture_asset] = std::move(result);

                ground_light_sprite_node_->SetLocalPosition(state_.groundLight.offset.value());
                ground_light_sprite_node_->SetLocalRotation(QuatFromEuler({270.0, 0.0, 0.0}));

                CreateQuadSettings settings;
                settings.size = state_.groundLight.size.value();
                auto sprite_mesh = GetView().GetMeshFactory().CreateQuad(settings);
                ground_light_sprite_node_->SetName("GroundLight");
                ground_light_sprite_node_->SetParent(GetNode());
                auto renderer_model = ground_light_sprite_node_->AddComponent<RenderComponent>(RenderComponent::FrustrumCullingMode::kDisabled);
                renderer_model->SetMesh(std::move(sprite_mesh));
                renderer_model->SetPriority(6);

                auto material = GetView().GetMaterialFactory().CreateMaterial(material_asset);
                material->SetParameter("BaseTexture", GetView().GetTextureFactory().CreateTexture(*texture_asset));
                material->SetParameter("Color", filament::RgbaType::sRGB, state_.groundLight.color.value()); // TODO: params
                material->SetParameter("Frequency", state_.groundLight.frequency.value());
                material->SetParameter("AlphaLowerMultiplier", state_.groundLight.alphaLowerMultiplier.value()); // TODO: params
                renderer_model->SetMaterial(std::move(material));
            }).KeptBy(this);
    }

    void Bonfire::AddSparks(SparksState state) {
        sparks_node_ = GetView().CreateNode();
        auto bonfire_particles_material_future = GetView().GetAssetManager().LoadMaterial(assets::kSparksMaterialCmat);
        auto fire_texture_future = GetView().GetAssetManager().LoadImage(assets::kTXSparkParticlesPng);

        bonfire_particles_material_future.Merge(fire_texture_future)
            .Then([this, state](std::tuple<AssetPtr<MaterialAsset>, AssetPtr<ImageAsset>> result) {
                auto [material_asset, texture_asset] = std::move(result);

                // Particle node
                sparks_node_->SetName("Sparks");
                sparks_node_->SetParent(GetNode());
                sparks_node_->SetLocalPosition(state.offset.value());

                // Particle material
                MaterialPtr ps_material = GetView().GetMaterialFactory().CreateMaterial(material_asset);
                auto ps_texture = GetView().GetTextureFactory().CreateTexture(*texture_asset);
                ps_material->SetParameter("BaseTexture", std::move(ps_texture));
                ps_material->SetParameter("BaseColor", filament::RgbaType::LINEAR, state.color.value());

                // Particle system setup
                auto options = ParticleSystemOptions();

                // Main options
                options.max_particles = 150;
                options.looping = false;

                // Renderer options
                options.mesh_type = ParticleMesh::QUAD;
                options.limit_update = true;
                options.emission_enabled = true;

                // Emitter options
                auto emitterOptions = EmitterOptions();
                emitterOptions.particle_life = state.particleLife.value();
                emitterOptions.emission_rate = state.emissionRate.value();
                emitterOptions.emission_quantity = 1;
                emitterOptions.is_emitter = true;
                float half_emitter_width = state.emitterWidth.value() / 2;
                emitterOptions.positionFunction = [=](NodeHandle node)
                {
                    return RandomValue(float3{ -half_emitter_width, 0.0, -2.0 },
                                       float3{ half_emitter_width, 1.0, 2.0 }).getValue();
                };

                emitterOptions.velocityFunction = [=]()
                {
                    return RandomValue(float3 { 0.0, state.minVelocity.value(), 0.0 },
                                       float3 { 0.0, state.maxVelocity.value(), 0.0}).getValue();
                };

                float half_angle = state.emissionConeAngle.value() / 2.0f;
                emitterOptions.rotationFunction = [=]()
                {
                    return RandomValue(float3 { 0.0, 0.0, -half_angle },
                                       float3 { 0.0, 0.0, half_angle }).getValue();
                };

                emitterOptions.scaleFunction = [state](){return state.particleScale.value();};

                // Particle system component
                auto particle_system = sparks_node_->AddComponent<ParticleSystem>(options);
                particle_system->AddEmitter(emitterOptions, float3(0), QuatFromEuler(float3(0)));
                particle_system->Initialize(std::move(ps_material));
                particle_system->Play();

            }).KeptBy(this);
    }

    float Bonfire::RandomNumber(float min, float max){
        auto generator = std::mt19937(rand());
        std::uniform_real_distribution<float> dist(min, max);
        return dist(generator);
    }

#if IMP_RUNTIME(DEV)
    // Customize Editor UI for this component
    void Bonfire::DrawEditorUi() {
        if (ImGui::Button("Reload effect"))
        {
            GetView().DestroyNode(flames_node_);
            GetView().DestroyNode(base_node_);
            GetView().DestroyNode(sparks_node_);
            GetView().DestroyNode(bonfire_sprite_node_);
            GetView().DestroyNode(ground_light_sprite_node_);

            Initialize();
        }
    }
#endif

}
