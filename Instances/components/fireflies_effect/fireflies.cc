#include "native/components/fireflies_effect/fireflies.h"
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>
#include "fireflies.h"

#define PI 3.1415

namespace ix::samsung::homecomponents {

    void Fireflies::Setup()
    {
        properties_ = SpawnProperties();
        Setup(properties_);
    }

    void Fireflies::Setup(SpawnProperties properties)
    {
        properties_ = properties;
        // Load material and image
        auto mat_future = GetView().GetAssetManager().LoadMaterial(assets::kFirefliesMaterialsCmat);
        auto img_future = GetView().GetAssetManager().LoadImage(assets::kFirefliesPng);

        mat_future.Merge(img_future).Then([=](std::tuple<imp::AssetPtr<imp::MaterialAsset>,
        imp::AssetPtr<imp::ImageAsset>> result) {

            auto [material, image] = result;
            // Create node and Component
            auto node = GetView().CreateNode();
            node->SetName("Fireflies_Component");
            node->SetParent(GetNode());
            node->SetLocalPosition(imp::float3(0.0f,0.0f,0.0f));

            // Particle material
            imp::MaterialPtr fireflies_material = GetView().GetMaterialFactory().CreateMaterial(material);
            auto ps_texture = GetView().GetTextureFactory().CreateTexture(*image);
            fireflies_material->SetParameter("BaseTexture", std::move(ps_texture));
            fireflies_material->SetParameter("FireflyMaxBaseColor", properties.firefly_max_base_color);
            fireflies_material->SetParameter("FireflyMinBaseColor", properties.firefly_min_base_color);
            fireflies_material->SetParameter("FireflyMinColor", properties.firefly_min_color);
            fireflies_material->SetParameter("MaxBrightness", properties.max_brightness);
            fireflies_material->SetParameter("MinBrightness", 0.0f);
            fireflies_material->SetParameter("MaxMovementRange", properties.max_movement_range);
            fireflies_material->SetParameter("MaxRandomBlikingTime", properties.max_random_time_to_blink);
            fireflies_material->SetParameter("MinRandomBlikingTime", properties.min_random_time_to_blink);
            fireflies_material->SetParameter("VelocityMultiplier", properties.velocity_multiplier);

            // Create and setup particle system
            auto options = ParticleSystemOptions();

            // Main options
            options.max_particles = properties.number_of_fireflies;
            options.initial_particles = properties.number_of_fireflies;
            options.limit_update = true;
            // Particle Parameters
            options.shader_size_multiplier = properties.shader_size_multiplier;

            auto emitterOptions = EmitterOptions();
            emitterOptions.initial_particles = properties.number_of_fireflies;
            emitterOptions.particle_life = 999999.0;

            emitterOptions.velocityFunction = []() {
                auto velocity = RandomValue(imp::float3 { 0.0 }, imp::float3 { 1.0 }).getValue();
                return imp::float3{
                    std::cos(velocity.x),
                    std::sin(velocity.y),
                    std::cos(velocity.z)};
            };
            emitterOptions.positionFunction = [](imp::NodeHandle){ 
                return RandomValue(imp::float3{-1.0, 0.0, -1.0}, imp::float3{1.0, 0.0, 1.0}).getValue();
            };
            emitterOptions.scaleFunction = [](){
                return RandomValue(imp::kOne3).getValue();
            };

            //add particle system component
            auto particle_system = node->AddComponent<ParticleSystem>(options);
            particle_system->Initialize(std::move(fireflies_material));
            particle_system->AddEmitter(emitterOptions);
            particle_system->Play();
        }).KeptBy(this);

    }

    float Fireflies::RandomNumber(float min, float max){
        generator_ = std::mt19937(rand());
        std::uniform_real_distribution<float> dist(min, max);
        return dist(generator_);
    }
}
