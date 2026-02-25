
#include "emitter.h"
#include "particle_system.h"

namespace ix::samsung::homecomponents
{

    void Emitter::Setup()
    {
    }

    void Emitter::Setup(EmitterOptions emitterOptions, ParticleSystem *particleSystem)
    {
        emitterOptions_ = emitterOptions;
        particleSystem_ = particleSystem;
        state_.emission_rate = emitterOptions_.emission_rate;
        state_.emission_quantity = emitterOptions_.emission_quantity;
        owner_ = GetNode();
    }

    void Emitter::Update(const imp::FrameTime &frame_time)
    {
        if (!particleSystem_->IsPlaying() || emitterOptions_.is_emitter == false)
            return;

        current_emission_time += std::clamp(frame_time.GetDeltaSeconds(), 1.0f / 90.0f, 1.0f / 30.0f);

        if (current_emission_time > emitterOptions_.emission_rate)
        {
            int quantity = emitterOptions_.emission_quantity * floor(current_emission_time / emitterOptions_.emission_rate);
            quantity = std::min<int>(quantity, particleSystem_->SizeAvailableParticles());
            EmitParticles(quantity);

            current_emission_time = 0;
        }
    }

    void Emitter::EmitInitialParticles()
    {
        EmitParticles(emitterOptions_.initial_particles);
    }

    void Emitter::EmitParticles(int quantity)
    {
        for (int i = 0; i < quantity; i++)
        {
            auto particle = particleSystem_->GetAvailableParticle();

            if (particle)
            {
                InitParticle(particle.get());
                particleSystem_->ApplyParametersToShader(particle.get());
            }
        }
    }

    void Emitter::InitParticle(Particle *particle)
    {
        particle->life = emitterOptions_.particle_life;
        particle->current_life = 0.0;
        particle->position = owner_->WorldFromLocalPoint(emitterOptions_.positionFunction(particle->node));
        particle->rotation = emitterOptions_.rotationFunction();
        particle->scale = emitterOptions_.scaleFunction();
        particle->velocity = owner_->WorldFromLocalVector(emitterOptions_.velocityFunction());
        particle->acceleration = emitterOptions_.accelerationFunction();

        particle->node->SetEnabled(true);
        particle->node->SetLocalScale(particle->scale);
        particle->node->SetWorldPosition(particle->position);
    }

    void Emitter::OnIsfStateChanged()
    {
        emitterOptions_.emission_rate = state_.emission_rate;
        emitterOptions_.emission_quantity = state_.emission_quantity;
        emitterOptions_.particle_life = state_.particle_life;
    }

} // namespace ix::samsung::homecomponents
