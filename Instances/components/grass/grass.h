#ifndef COMPONENTS_GRASS_H
#define COMPONENTS_GRASS_H

#include "imp.h"
#include "absl/status/status.h"
#include "core/ncsb/component.h"
#include "proto/components/grass_state.proto.imp.h"
#include "native/components/particle_instances/particle_instances.h"

namespace ix::samsung::homecomponents
{
    struct GrassData {
    public:
        int amount =              100;
        float4 color1 =            float4(0.083, 0.147, 0.025, 1.0);
        float4 color2 =            float4(0.083, 0.147, 0.025, 1.0);

        // wind
        float wind_frequency =    9.0f;
        float wave_size =         0.5f;

        // LOD / MipMap
        float lod_min_distance =  5.0f;
        float lod_max_distance =  20.0f;
    };

    class Grass : public imp::Component
    {
    private:
        imp::Material *particle_material_ptr_;
        imp::NodeHandle particle_node_;
        GrassState state_;
        GrassData grass_data_ = {};

    public:
        void Setup();
        void InitializeParticles();
        void InstanceParticles();
        void ResetEffect();

        using IsfInfo = imp::IsfInfo<&Grass::state_>;
        void OnIsfStateChanged();

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };

}  // namespace ix::samsung::homecomponents

#endif // COMPONENTS_GRASS_H
