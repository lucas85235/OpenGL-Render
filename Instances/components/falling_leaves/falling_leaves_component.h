#ifndef COMPONENTS_FALLING_LEAVES_H
#define COMPONENTS_FALLING_LEAVES_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/falling_leaves_state.proto.imp.h"
#include "core/view/framework/render/material.h"
#include "native/components/particle_instances/particle_instances.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    struct FallingLeavesData {
    public:
        int amount;
        float velocity;
        float size;
        float length;
        float3 first_color_01;
        float3 second_color_01;
        float3 first_color_02;
        float3 second_color_02;
        float3 first_color_03;
        float3 second_color_03;
        float3 position;
        float3 rotation;
    };

    class FallingLeavesComponent : public imp::Component
    {
    private:
        NodeHandle particle_instance_node_, particle_instance_child_node_;
        FallingLeavesData fallingLeavesData_;
        FallingLeavesState state_;

    public:
        void Setup();
        void InitializeFallingLeaves();
        void InstanceFallingLeaves(FallingLeavesData fld_);
        void DestroyFallingLeaves();


        using IsfInfo = imp::IsfInfo<&FallingLeavesComponent::state_>;

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };
}

#endif // COMPONENTS_FALLING_LEAVES_H