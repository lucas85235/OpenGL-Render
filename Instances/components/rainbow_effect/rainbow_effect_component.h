#ifndef COMPONENTS_RAINBOW_EFFECT_H
#define COMPONENTS_RAINBOW_EFFECT_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/rainbow_effect_state.proto.imp.h"
#include "core/view/framework/render/material.h"

using namespace imp;

namespace ix::samsung::homecomponents {
    struct RainbowEffectData {
    public:
        float rainbowRingsThickness;
        float rainbowRingsSmoothness;
        float rainbowRadiusSmoothness;
        float rainbowStartRenderingAngle;
        float rainbowEndRenderingAngle;
        float fadeStartAngle;
        float fadeEndAngle;
        float colorIntensity;
        float opacity;
        float3 position;
        float3 rotation;
        float3 scale;
    };

    class RainbowEffectComponent : public imp::Component {
    private:
        NodeHandle node;
        RainbowEffectData rainbowEffectData_;
        RainbowEffectState state_;

    public:
        void Setup();

        void InitializeRainbowEffect();

        void InstanceRainbowEffect(RainbowEffectData rbw_);

        void DestroyRainbowEffect();

        using IsfInfo = imp::IsfInfo<&RainbowEffectComponent::state_>;

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };
}

#endif // COMPONENTS_RAINBOW_EFFECT_H
