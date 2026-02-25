#ifndef FOG_STATE_MACHINE_H
#define FOG_STATE_MACHINE_H

#include "native/components/particle_system/particle_system.h"

namespace ix::samsung::homecomponents
{
    class SandWindStateMachine
    {
        enum SandWindState {
            FADING_IN,
            RUNNING,
            FADING_OUT,
            STOPPED
        };

    private:
        float elapsed_ = 0;
        SandWindState current_state_ = FADING_IN;
        const std::string_view FADE = "Fade";

    public:
        float fading_time_ = 2.0f;
        float running_time_ = 5.0f;
        float stopped_time_ = 5.0f;

        imp::ComponentHandle<ParticleSystem> particle_system_;
        imp::ComponentHandle<ParticleSystem> dust_particle_system_;
        imp::Material* material_;

        SandWindStateMachine();

        void StartEmission(imp::ComponentHandle<ParticleSystem>& particle_system);
        void PauseEmission(imp::ComponentHandle<ParticleSystem>& particle_system);

        void SetMaterialFadeParameter(float value);
        void ChangeState(SandWindState new_state);
        void Update(float delta);
        void Reset();
    };
}
#endif