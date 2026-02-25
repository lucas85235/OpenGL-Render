#include "imp.h"
#include "native/components/sand_wind/sand_wind.h"
#include "native/components/particle_system/particle_system.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    SandWindStateMachine::SandWindStateMachine()
    {
        material_ = nullptr;
        Reset();
    }

    void SandWindStateMachine::ChangeState(const SandWindState new_state)
    {
        if(new_state == FADING_IN)
        {
            StartEmission(particle_system_);
            StartEmission(dust_particle_system_);
        }
        else if(new_state == FADING_OUT)
        {
            PauseEmission(particle_system_);
            PauseEmission(dust_particle_system_);
        }
        current_state_ = new_state;
        elapsed_ = 0;
    }

    void SandWindStateMachine::Update(const float delta)
    {
        switch(current_state_)
        {
            case FADING_IN:
                SetMaterialFadeParameter(elapsed_ / fading_time_);
                if(elapsed_ > fading_time_)
                {
                    SetMaterialFadeParameter(1.0f);
                    ChangeState(RUNNING);
                }
                elapsed_ += delta;
                break;

            case RUNNING:
                if(elapsed_ > running_time_)
                {
                    ChangeState(FADING_OUT);
                }
                elapsed_ += delta;
                break;

            case FADING_OUT:
                SetMaterialFadeParameter(1.0f - (elapsed_ / fading_time_));
                if(elapsed_ > fading_time_)
                {
                    SetMaterialFadeParameter(0);
                    ChangeState(STOPPED);
                }
                elapsed_ += delta;
                break;

            case STOPPED:
                if(elapsed_ > stopped_time_)
                {
                    ChangeState(FADING_IN);
                }
                elapsed_ += delta;
                break;
        }
    }

    void SandWindStateMachine::Reset()
    {
        current_state_ = FADING_IN;
        elapsed_ = 0;
        if (material_) material_->SetParameter(FADE, 0.0f);
    }

    void SandWindStateMachine::StartEmission(imp::ComponentHandle<ParticleSystem>& particle_system)
    {
        if (!particle_system) return;
        particle_system->StartEmission();
    }

    void SandWindStateMachine::PauseEmission(imp::ComponentHandle<ParticleSystem>& particle_system)
    {
        if (!particle_system) return;
        particle_system->PauseEmission();
    }

    void SandWindStateMachine::SetMaterialFadeParameter(float value)
    {
        if (material_) material_->SetParameter(FADE, value);
    }

}
