#include "native/components/snowflake/snowflake.h"
#include "native/components/particle_system/shape_utils.h"
#include "native/components/snowflake/snowflake_assets.h"
#include "native/components/particle_instances/particle_instances.h"


namespace ix::samsung::homecomponents
{
    enum class RotationAxis
    {
        X,
        Y,
    };

    void Snowflake::Setup()
    {
        InitializeSnowflakes();
    }

    void Snowflake::InitializeSnowflakesParameters()
    {
        // Top Particle Parameters
        snowflake_data_.topParticleAmount = state_.topIntensity.value_or(snowflake_data_.topParticleAmount);
        snowflake_data_.topParticleMinScale = state_.topMinScale.value_or(snowflake_data_.topParticleMinScale);
        snowflake_data_.topParticleMaxScale = state_.topMaxScale.value_or(snowflake_data_.topParticleMaxScale);
        snowflake_data_.topAlpha = state_.topAlpha.value_or(snowflake_data_.topAlpha);
        snowflake_data_.topParticleColor = state_.topParticleColor.value_or(snowflake_data_.topParticleColor);
        snowflake_data_.topMinVelocity = state_.topMinVelocity.value_or(snowflake_data_.topMinVelocity);
        snowflake_data_.topMaxVelocity = state_.topMaxVelocity.value_or(snowflake_data_.topMaxVelocity);

        // Side Particle Parameters
        snowflake_data_.sideParticleAmount = state_.sideIntensity.value_or(snowflake_data_.sideParticleAmount);
        snowflake_data_.sideParticleMinScale = state_.sideMinScale.value_or(snowflake_data_.sideParticleMinScale);
        snowflake_data_.sideParticleMaxScale = state_.sideMaxScale.value_or(snowflake_data_.sideParticleMaxScale);
        snowflake_data_.sideAlpha = state_.sideAlpha.value_or(snowflake_data_.sideAlpha);
        snowflake_data_.sideParticleColor = state_.sideParticleColor.value_or(snowflake_data_.sideParticleColor);
        snowflake_data_.sideMinVelocity = state_.sideMinVelocity.value_or(snowflake_data_.sideMinVelocity);
        snowflake_data_.sideMaxVelocity = state_.sideMaxVelocity.value_or(snowflake_data_.sideMaxVelocity);
        InitializeSnowflakes();
    }

    void Snowflake::ResetSnowflakesParameters()
    {
        // Top Particle Parameters
        snowflake_data_.topParticleAmount = state_.topIntensity.emplace(initial_values_snowflake_data_.topParticleAmount);
        snowflake_data_.topParticleMinScale = state_.topMinScale.emplace(initial_values_snowflake_data_.topParticleMinScale);
        snowflake_data_.topParticleMaxScale = state_.topMaxScale.emplace(initial_values_snowflake_data_.topParticleMaxScale);
        snowflake_data_.topAlpha = state_.topAlpha.emplace(initial_values_snowflake_data_.topAlpha);
        snowflake_data_.topParticleColor = state_.topParticleColor.emplace(initial_values_snowflake_data_.topParticleColor);
        snowflake_data_.topMinVelocity = state_.topMinVelocity.emplace(initial_values_snowflake_data_.topMinVelocity);
        snowflake_data_.topMaxVelocity = state_.topMaxVelocity.emplace(initial_values_snowflake_data_.topMaxVelocity);

        // Side Particle Parameters
        snowflake_data_.sideParticleAmount = state_.sideIntensity.emplace(initial_values_snowflake_data_.sideParticleAmount);
        snowflake_data_.sideParticleMinScale = state_.sideMinScale.emplace(initial_values_snowflake_data_.sideParticleMinScale);
        snowflake_data_.sideParticleMaxScale = state_.sideMaxScale.emplace(initial_values_snowflake_data_.sideParticleMaxScale);
        snowflake_data_.sideAlpha = state_.sideAlpha.emplace(initial_values_snowflake_data_.sideAlpha);
        snowflake_data_.sideParticleColor = state_.sideParticleColor.emplace(initial_values_snowflake_data_.sideParticleColor);
        snowflake_data_.sideMinVelocity = state_.sideMinVelocity.emplace(initial_values_snowflake_data_.sideMinVelocity);
        snowflake_data_.sideMaxVelocity = state_.sideMaxVelocity.emplace(initial_values_snowflake_data_.sideMaxVelocity);
        InitializeSnowflakes();
    }

    void Snowflake::InitializeSnowflakes()
    {
        auto snow_mat_quad = GetView().GetAssetManager().LoadMaterial(assets::kSnowflakeMaterialCmat);
        auto snow_mat_sphere = GetView().GetAssetManager().LoadMaterial(assets::kSnowflakeMaterialSphereCmat);
        auto snow_tex = GetView().GetAssetManager().LoadImage(assets::kTXSnowParticlePng);
        auto snow_tex2 = GetView().GetAssetManager().LoadImage(assets::kTXSnowParticleSolid50Png);
        auto snow_tex3 = GetView().GetAssetManager().LoadImage(assets::kTXSnowParticleCapPng);

        snow_mat_quad.Merge(snow_tex, snow_tex2, snow_tex3, snow_mat_sphere).Then([this](std::tuple<imp::AssetPtr<imp::MaterialAsset>,
                                                        imp::AssetPtr<imp::ImageAsset>, imp::AssetPtr<imp::ImageAsset>,
                                                        imp::AssetPtr<imp::ImageAsset>, imp::AssetPtr<imp::MaterialAsset>> result)
        {
            auto [materialQuad, snowImage1, snowImage2, snowImage3, materialSphere] = result;

            MeshQuad mesh_quad;

            //Create Top Particles System
            CreateSnowflakes(materialQuad, snowImage1, &mesh_quad,
                snowflake_data_.topParticleAmount / 2,
                snowflake_data_.topPositionOffsetX,
                snowflake_data_.topPositionOffsetY,
                snowflake_data_.topPositionOffsetZ,
                RotationAxis::X,
                1.0,
                snowflake_data_.topMaxRadius,
                snowflake_data_.topParticleMinScale,
                snowflake_data_.topParticleMaxScale,
                snowflake_data_.topLifeTime, 1.67,
                float4(snowflake_data_.topParticleColor, snowflake_data_.topAlpha),
                snowflake_data_.topMinVelocity,
                snowflake_data_.topMaxVelocity,
                snowflake_data_.topMinNoiseSpeed,
                snowflake_data_.topMaxNoiseSpeed,
                snowflake_data_.topMinNoiseAmplitude,
                snowflake_data_.topMaxNoiseAmplitude,
                snowflake_data_.topMinNoiseFrequency,
                snowflake_data_.topMaxNoiseFrequency
                );

            CreateSnowflakes(materialQuad, snowImage2, &mesh_quad,
                snowflake_data_.topParticleAmount / 2,
                snowflake_data_.topPositionOffsetX,
                snowflake_data_.topPositionOffsetY,
                snowflake_data_.topPositionOffsetZ,
                RotationAxis::X,
                1.0,
                snowflake_data_.topMaxRadius,
                snowflake_data_.topParticleMinScale,
                snowflake_data_.topParticleMaxScale,
                snowflake_data_.topLifeTime, 1.67,
                float4(snowflake_data_.topParticleColor, snowflake_data_.topAlpha),
                snowflake_data_.topMinVelocity,
                snowflake_data_.topMaxVelocity,
                snowflake_data_.topMinNoiseSpeed,
                snowflake_data_.topMaxNoiseSpeed,
                snowflake_data_.topMinNoiseAmplitude,
                snowflake_data_.topMaxNoiseAmplitude,
                snowflake_data_.topMinNoiseFrequency,
                snowflake_data_.topMaxNoiseFrequency
                );

            //Create Around Particles System
            CreateSnowflakes(materialQuad, snowImage1, &mesh_quad,
                snowflake_data_.sideParticleAmount / 2,
                snowflake_data_.sidePositionOffsetX,
                snowflake_data_.sidePositionOffsetY,
                snowflake_data_.sidePositionOffsetZ,
                RotationAxis::Y,
                snowflake_data_.sideMinRadius,
                snowflake_data_.sideMaxRadius,
                snowflake_data_.sideParticleMinScale,
                snowflake_data_.sideParticleMaxScale,
                snowflake_data_.  sideLifeTime, 0.0,
                float4(snowflake_data_.sideParticleColor, snowflake_data_.sideAlpha),
                snowflake_data_.sideMinVelocity,
                snowflake_data_.sideMaxVelocity,
                snowflake_data_.sideMinNoiseSpeed,
                snowflake_data_.sideMaxNoiseSpeed,
                snowflake_data_.sideMinNoiseAmplitude,
                snowflake_data_.sideMaxNoiseAmplitude,
                snowflake_data_.sideMinNoiseFrequency,
                snowflake_data_.sideMaxNoiseFrequency
                );

            CreateSnowflakes(materialQuad, snowImage2, &mesh_quad,
                snowflake_data_.sideParticleAmount / 2,
                snowflake_data_.sidePositionOffsetX,
                snowflake_data_.sidePositionOffsetY,
                snowflake_data_.sidePositionOffsetZ,
                RotationAxis::Y,
                snowflake_data_.sideMinRadius,
                snowflake_data_.sideMaxRadius,
                snowflake_data_.sideParticleMinScale,
                snowflake_data_.sideParticleMaxScale,
                snowflake_data_.sideLifeTime, 0.0,
                float4(snowflake_data_.sideParticleColor, snowflake_data_.sideAlpha),
                snowflake_data_.sideMinVelocity,
                snowflake_data_.sideMaxVelocity,
                snowflake_data_.sideMinNoiseSpeed,
                snowflake_data_.sideMaxNoiseSpeed,
                snowflake_data_.sideMinNoiseAmplitude,
                snowflake_data_.sideMaxNoiseAmplitude,
                snowflake_data_.sideMinNoiseFrequency,
                snowflake_data_.sideMaxNoiseFrequency
                );
        }).KeptBy(this);

    }

    void Snowflake::CreateSnowflakes
    (
        imp::AssetPtr<imp::MaterialAsset> material, imp::AssetPtr<imp::ImageAsset> image, MeshInfo* mesh_info, float amount, float posOffsetX, float posOffsetY, float posOffsetZ,
        RotationAxis rotation_axis, float minRadius, float maxRadius, float minScale, float maxScale, float lifeTime, float radianAngle, imp::float4 color, float minVelocity, float maxVelocity,
        float minNoiseSpeed, float maxNoiseSpeed, float minNoiseAmplitude, float maxNoiseAmplitude, float minNoiseFrequency, float maxNoiseFrequency)
    {
        auto particle_node = GetView().CreateNode();
        auto image_ptr = GetView().GetTextureFactory().CreateTexture(*image);
        auto params = ParticleInstancesParams();
        params.amount = amount;
        params.material = GetView().GetMaterialFactory().CreateMaterial(material);

        params.material -> SetParameter("BaseTexture", std::move(image_ptr));
        params.material -> SetParameter("PositionOffsetX",             posOffsetX);
        params.material -> SetParameter("PositionOffsetY",             posOffsetY);
        params.material -> SetParameter("PositionOffsetZ",             posOffsetZ);

        params.material -> SetParameter("RotationAxis", static_cast<int>(rotation_axis));
        params.material -> SetParameter("LifeTime",                    lifeTime);
        params.material -> SetParameter("MinScale",                    minScale);
        params.material -> SetParameter("MaxScale",                    maxScale);
        params.material -> SetParameter("MinNoiseSpeed",               minNoiseSpeed);
        params.material -> SetParameter("MaxNoiseSpeed",               maxNoiseSpeed);
        params.material -> SetParameter("MinNoiseAmplitude",           minNoiseAmplitude);
        params.material -> SetParameter("MaxNoiseAmplitude",           maxNoiseAmplitude);
        params.material -> SetParameter("MinNoiseFrequency",           minNoiseFrequency);
        params.material -> SetParameter("MaxNoiseFrequency",           maxNoiseFrequency);
        params.material -> SetParameter("MinVelocity",                 minVelocity);
        params.material -> SetParameter("MaxVelocity",                 maxVelocity);
        params.material -> SetParameter("Colour",                      color);

        params.mesh = mesh_info;
        auto cone = ConeV4Angle(minRadius, maxRadius, lifeTime, radianAngle);
        params.positionFunction = cone.get();

        particle_node->SetName("GPUParticles");
        particle_node->SetParent(GetNode());
        particle_instance_ = particle_node->AddComponent<ParticleInstances>(params);
        particle_instance_nodes_.push_back(particle_node);
    }

    void Snowflake::UpdateSnowflakes() {

        for(auto node : particle_instance_nodes_)
        {
            GetView().DestroyNode(node);
        }
        InitializeSnowflakesParameters();
    }

    void Snowflake::ResetSnowflakes() {

        for(auto node : particle_instance_nodes_)
        {
            GetView().DestroyNode(node);
        }
        ResetSnowflakesParameters();
    }

#if IMP_RUNTIME(DEV)
    void Snowflake::DrawEditorUi()
    {
        if (ImGui::Button("Update Settings"))
        {
            UpdateSnowflakes();
        }

        if (ImGui::Button("Reset Settings"))
        {
            ResetSnowflakes();
        }
    }
#endif

} // ix::moohan::home_support