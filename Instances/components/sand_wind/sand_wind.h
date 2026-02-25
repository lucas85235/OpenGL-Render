#ifndef COMPONENTS_SAND_WIND_H
#define COMPONENTS_SAND_WIND_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/sand_wind_state.proto.imp.h"
#include "native/components/sand_wind/sand_wind_state_machine.h"

namespace ix::samsung::homecomponents
{
    class SandWind : public imp::Component
    {
    private:
        imp::Material *layer_material_ptr_;
        imp::Material *particle_material_ptr_;
        imp::Material *dust_particle_material_ptr_;
        SandWindState state_;
        imp::NodeHandle particle_node_;
        imp::NodeHandle layer_node_;
        imp::NodeHandle dust_node_;
        SandWindStateMachine state_machine_;

    public:
        void Setup();
        using IsfInfo = imp::IsfInfo<&SandWind::state_>;
        void OnIsfStateChanged();
        void Update(const imp::FrameTime& frame_time);
        void InitializeNoise();
        void InitializeParticles();
        void InitializeDust();
        void InitializeFadeCycle();

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };
}

#endif // COMPONENTS_SAND_WIND_H
