#ifndef COMPONENTS_AURORA_H
#define COMPONENTS_AURORA_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/aurora_state.proto.imp.h"
#include "core/view/framework/render/material.h"

namespace ix::samsung::homecomponents
{
    struct AuroraParameters
    {
        imp::float4 color_1 = { 0.255, 0.918, 0.459, 1.0 };
        imp::float4 color_2 = { 0.337, 0.294, 0.804, 1.0 };
        imp::float4 color_3 = { 0.000, 0.623, 0.192, 1.0 };
        float noise_strength = 0.11f;
        float gradient_speed = 0.35f;
        float vertical_noise_strength = 0.25f;
        AuroraState::Type type = AuroraState::Type::TYPE_1;

    };

    class Aurora : public imp::Component
    {
    private:
        imp::Material *material_ptr_;
        AuroraState state_;
        AuroraState::Type current_type_ = AuroraState::Type::NONE;
        AuroraState::NoiseType current_noise_type_ = AuroraState::NoiseType::NOISE_TYPE_1;

    public:
        void Setup();
        void Setup(AuroraParameters parameters);
        void Initialize();
        void ChangeAssetType();
        void ChangeNoiseType();

        using IsfInfo = imp::IsfInfo<&Aurora::state_>;
        void OnIsfStateChanged();
    };
}

#endif // COMPONENTS_AURORA_H
