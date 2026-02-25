#ifndef COMPONENTS_TWINKLE_STARS_H
#define COMPONENTS_TWINKLE_STARS_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/twinkle_stars_state.proto.imp.h"
#include "native/components/particle_system/particle_system.h"
#include "native/components/particle_instances/particle_instances.h"
#include "core/view/framework/render/material.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    struct TwinkleStarsData {
    public:
        int id;
        int amount;
        float radius;
        float size;
        float2 radial;
        float2 angular;
        float3 position;
        float3 rotation;
        float3 color;
        float velocity;
    };

    class TwinkleStarsComponent : public imp::Component
    {
    private:
        NodeHandle particle_instance_node_, particle_instance_child_node_;
        TwinkleStarsData twinkleStarsData_;
        TwinkleStarsState state_;

    public:
        void Setup();
        void InitializeStars();
        void InstanceStars(TwinkleStarsData tsd_);
        void DestroyStars();

        using IsfInfo = imp::IsfInfo<&TwinkleStarsComponent::state_>;

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };
}

#endif // COMPONENTS_SUNSHINE_H