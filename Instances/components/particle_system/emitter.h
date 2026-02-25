#pragma once

#include "imp.h"
#include "core/ncsb/component.h"
#include "proto/components/emitter_state.proto.imp.h"

#include "particle.h"

namespace ix::samsung::homecomponents
{

    class ParticleSystem;

    typedef std::function<imp::float3()> Function3f;

    struct EmitterOptions
    {
        float emission_rate = 0.01f;
        int emission_quantity = 1;
        int initial_particles = 0; // Emit X particles in the first frame
        float particle_life = 1.0;
        bool is_emitter = false; // Is used to determine whether particle emission should continue after the first emission

        std::function<imp::float3(imp::NodeHandle)> positionFunction = [](imp::NodeHandle n){return imp::float3(0.0);};
        Function3f rotationFunction = []{return imp::float3(0.0);};
        Function3f scaleFunction = []{return imp::float3(0.1);};
        Function3f velocityFunction = []{return imp::float3(0.0);};
        Function3f accelerationFunction = []{return imp::float3(0.0);};
    };

    class Emitter : public imp::Component
    {
    private:
        void InitParticle(Particle *particle);
        void EmitParticles(int quantity);
        ParticleSystem *particleSystem_;
        EmitterOptions emitterOptions_;
        EmitterState state_;
        imp::NodeHandle owner_;

    public:
        void Setup();
        void Setup(EmitterOptions emitterOptions, ParticleSystem *particleSystem);
        void Update(const imp::FrameTime &frame_time);

        void EmitInitialParticles();
        using IsfInfo = imp::IsfInfo<&Emitter::state_>;
        void OnIsfStateChanged();

        float current_emission_time = 0.0f;

        friend class ParticleSystem;
    };
}
