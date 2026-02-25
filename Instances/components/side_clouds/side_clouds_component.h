#ifndef COMPONENTS_SIDE_CLOUDS_H
#define COMPONENTS_SIDE_CLOUDS_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/side_clouds_state.proto.imp.h"
#include "core/view/framework/render/material.h"

namespace ix::samsung::homecomponents
{
    struct SideCloudParams {
        // Material parameters, with default values
        float kDensity = 1.10f;
        float kVelocity = 0.2f;
        float kSamples = 3.0f;
        float kSmallCloudsSpeed = 0.2f;
        imp::float4 kColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    class SideCloudsComponent : public imp::Component
    {
    private:
        SideCloudsState state_;
        imp::MaterialPtr material_ptr_;
        imp::ComponentHandle<imp::RenderComponent> renderer_;

    public:
        using IsfInfo = imp::IsfInfo<&SideCloudsComponent::state_>;

        void Setup();
        void Setup(SideCloudParams& params);
        void InitializeSideClouds();
        void OnIsfStateChanged();
        void VisitNodeTreeToDisableGltfColliders(imp::NodeHandle node);
    };
}

#endif // COMPONENTS_CLOUDS_H
