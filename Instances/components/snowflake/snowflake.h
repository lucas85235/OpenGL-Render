#ifndef COMPONENTS_SNOWFLAKE_H
#define COMPONENTS_SNOWFLAKE_H

#include "imp.h"
#include "native/components/particle_instances/particle_instances.h"
#include <vector>

#include "proto/components/snowflake_state.proto.imp.h"
#include "native/components/particle_system/particle_system.h"

namespace ix::samsung::homecomponents
{
    struct SnowflakeData
    {
        float sideLifeTime         =  45.0;
        float sidePositionOffsetX  =  5.0;
        float sidePositionOffsetY  =  -5.00;
        float sidePositionOffsetZ  =  1.0;

        //Proto Parameters
        float topLifeTime           =  10.85;
        float topPositionOffsetX    =  6.85;
        float topPositionOffsetY    =  19.00;
        float topPositionOffsetZ    =  -3.78;

        int topParticleShape        =  1;
        int topParticleAmount       =  2100;
        float topParticleMinScale   =  0.04;
        float topParticleMaxScale   =  0.06;
        float topMaxRadius          =  20;
        float topAlpha              =  0.3;
        float3 topParticleColor     =  float3(0.859,0.890,0.919);

        float topMinVelocity        =  0.08;
        float topMaxVelocity        =  0.10;
        float topMinNoiseSpeed      =  1.0;
        float topMaxNoiseSpeed      =  2.0;
        float topMinNoiseAmplitude  =  0.1;
        float topMaxNoiseAmplitude  =  2.0;
        float topMinNoiseFrequency  =  0.03;
        float topMaxNoiseFrequency  =  0.1;

        int    sideParticleShape    =  1;
        int    sideParticleAmount   =  10000;
        float  sideParticleMinScale =  0.06;
        float  sideParticleMaxScale =  0.10;
        float  sideMinRadius        =  25;
        float  sideMaxRadius        =  40;
        float  sideAlpha            =  0.3;
        float3 sideParticleColor    =  float3(1.0,1.0,1.0);

        float sideMinVelocity       =  0.10;
        float sideMaxVelocity       =  0.13;
        float sideMinNoiseSpeed     =  1.0;
        float sideMaxNoiseSpeed     =  1.0;
        float sideMinNoiseAmplitude =  2.00;
        float sideMaxNoiseAmplitude =  3.00;
        float sideMinNoiseFrequency =  0.02;
        float sideMaxNoiseFrequency =  0.05;
    };

    enum class RotationAxis;

    class Snowflake : public imp::Component
    {
        ComponentHandle<ParticleSystem> particle_system_;
        std::vector<NodeHandle> particle_instance_nodes_;
        ComponentHandle<ParticleInstances> particle_instance_;
        SnowflakeData snowflake_data_;
        SnowflakeData initial_values_snowflake_data_;
        SnowflakeState state_;
        float4 RingPositionAngle(float minRadius, float maxRadius,  float max_height, float radianAngle);



    public:
        void Setup();
        void InitializeSnowflakesParameters();
        void ResetSnowflakesParameters();
        void InitializeSnowflakes();
        void CreateSnowflakes(
            imp::AssetPtr<imp::MaterialAsset> material, imp::AssetPtr<imp::ImageAsset> image, MeshInfo* mesh_info, float amount, float posOffsetX, float posOffsetY, float posOffsetZ,RotationAxis rotation_axis,
            float minRadius, float maxRadius, float max_height, float minScale, float maxScale, float radianAngle, imp::float4 color, float minVelocity, float maxVelocity,
           float minNoiseSpeed, float maxNoiseSpeed, float minNoiseAmplitude, float maxNoiseAmplitude, float minNoiseFrequency, float maxNoiseFrequency);
        void UpdateSnowflakes();
        void ResetSnowflakes();
        using IsfInfo = imp::IsfInfo<&Snowflake::state_>;

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };
} // namespace xr::component

#endif // COMPONENTS_SNOWFLAKE_h