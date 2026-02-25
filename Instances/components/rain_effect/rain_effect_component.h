#ifndef COMPONENTS_RAIN_EFFECT_H
#define COMPONENTS_RAIN_EFFECT_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/rain_effect_state.proto.imp.h"
#include "native/components/particle_system/particle_system.h"
#include "native/components/particle_instances/particle_instances.h"
#include "core/view/framework/render/material.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    struct RainEffectData {
    public:
        int id;
        int amount;
        float radius;
        float size;
        float velocity;
        int angle;
    };

    class RainEffectComponent : public imp::Component
    {
    private:
        NodeHandle particle_instance_node_, particle_instance_child_node_;
        RainEffectData rainEffectData_;
        RainEffectState state_;

    public:
        void Setup();
        void InitializeRain();
        void InstanceRains(RainEffectData red_);
        void DestroyRain();

        using IsfInfo = imp::IsfInfo<&RainEffectComponent::state_>;

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };
}

#endif // COMPONENTS_RAIN_EFFECT_H