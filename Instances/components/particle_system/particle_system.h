#pragma once

#include "imp.h"
#include "core/ncsb/component.h"

#include <iostream>
#include <type_traits>

#include <random>
#include <list>
#include <queue>

#include "random_value.h"
#include "particle.h"
#include "emitter.h"

namespace ix::samsung::homecomponents {

    enum class PositionShapes {
        DEFAULT,
        SPHERE,
        CIRCLE,
        UP_CIRCLE
    };

    enum class VelocityType {
        DEFAULT,
        COS_SIN_COS,
    };

    enum class ScaleType {
        DEFAULT,
        UNIFORM,
    };

    enum class ParticleMesh {
        QUAD,
        BOX,
        SPHERE,
        CUSTOM,
    };

    enum class EmissionShape {
        QUAD,
        SPHERE,
    };

    struct ParticleSystemOptions {
        // Main
        size_t max_particles = 100;
        size_t initial_particles = 0;
        float particle_life = 1.0;
        bool looping = true;

        // Renderer
        ParticleMesh mesh_type = ParticleMesh::QUAD;
        EmissionShape emission_shape = EmissionShape::QUAD;
        imp::RenderComponent::BlendOrderMode blend_order_mode = imp::RenderComponent::BlendOrderMode::kGlobal;
        int render_priority = 0;
        bool limit_update;

        // This is the shader size usually for uv, but you can use on vertex or otherwise
        float shader_size_multiplier = 1.0;

        // Parameters
        PositionShapes position_type = PositionShapes::DEFAULT;
        VelocityType velocity_type = VelocityType::DEFAULT;
        ScaleType scale_type = ScaleType::DEFAULT;

        RandomValue<imp::float4> color = RandomValue(imp::float4 { 0.0, 0.0, 0.0, 0.0 }, imp::float4 { 1.0, 1.0, 1.0, 1.0 });
        RandomValue<imp::float3> position = RandomValue(imp::float3{-1.0, -1.0, -1.0}, imp::float3{1.0, 1.0, 1.0});
        RandomValue<imp::float3> velocity = RandomValue(imp::float3 { 0.0, 0.0, 0.0 }, imp::float3 { 1.0, 1.0, 1.0 });
        RandomValue<imp::float3> acceleration = RandomValue(imp::float3 { 0.0, 0.0, 0.0 }, imp::float3 { 1.0, 1.0, 1.0 });
        RandomValue<imp::float3> scale = RandomValue(imp::float3{0.2f, 0.2f, 0.2f}, imp::float3{0.5f, 0.5f, 0.5f}); // This is the object scale setterof the object
        RandomValue<imp::float3> noise = RandomValue(imp::float3 { 0.0, 0.0, 0.0 }, imp::float3 { 1.0, 1.0, 1.0 });
        RandomValue<imp::float3> rotation = RandomValue(imp::float3 { 0.0, 0.0, 0.0 }, imp::float3 { 1.0, 1.0, 1.0 });
        RandomValue<float> period = RandomValue(0.0f, 1.0f);
        RandomValue<float> amplitude = RandomValue(0.0f, 1.0f);

        // Emission
        bool emission_enabled = false;
        float emission_rate = 0.1f;
    };

    class ParticleSystem : public imp::Component
    {
    private:
        const float FIXED_TIME_ = 0.016f;
        bool playing_ = false;
        std::list<std::shared_ptr<Particle>> particles_;

        std::mt19937 generator; // remove this and use the random between generator
        float count_update_ = 0.0f;
        float current_time_ = 0.0f;

        // Particle object pool
        // This pool implementation uses a vector as the actual storage for the particles and a
        // queue for storing the available particles at any moment.
        // When a particle is needed, an index is obtained from the queue and used to retrieve
        // the particle from the vector, which is then properly initialized.
        // When a particle is disabled, its node must be disabled and its pool index must pushed
        // into the queue for later reuse.
        std::vector<std::shared_ptr<Particle>> particle_pool_;
        std::queue<int> particle_indexes_;

        std::vector<Emitter*> emitters;

        // Main options
        ParticleSystemOptions options_;

        // Emission options
        bool emission_enabled_ = false;
        float emission_rate = 0.1f;
        float current_emission_time = 0;

        // Renderer options
        imp::MaterialPtr material_;
        imp::MeshPtr mesh_;
        imp::MeshPtr custom_mesh_;

        imp::NodeHandle CreateShapeNode(Particle* particle);
        imp::MeshPtr CreateParticleMesh(ParticleMesh mesh_type);
        imp::float3 CalculateVelocity(VelocityType type);
        imp::float3 CalculateAcceleration();
        imp::float3 CalculateRotation();
        imp::float3 CalculateScale(ScaleType type);
        imp::float3 CalculatePositionShapes(PositionShapes type, imp::NodeHandle node);
        float CalculateAmplitude();
        float CalculatePeriod();

        void FillParticlePool();
        void ParticleCreate();
        void ParticleUpdate(float delta_time);

    public:
        void AddEmitter(EmitterOptions e, imp::float3 position = imp::kZero3, imp::quatf rotation = imp::kIdentityQuatf);
        std::shared_ptr<Particle> GetAvailableParticle();
        int SizeAvailableParticles();
        void ApplyParametersToShader(Particle* particle);
        bool IsPlaying();
        void Initialize(imp::MaterialPtr material);
        void DestroyAllParticles();
        void Setup(ParticleSystemOptions options);
        void Update(const imp::FrameTime& frame_time);

        void Play();
        void Pause();
        void Stop();
        void PauseEmission();
        void StartEmission();

        void SetCustomMesh(imp::MeshPtr custom_mesh);
    };

} // namespace ix::samsung::homecomponents
