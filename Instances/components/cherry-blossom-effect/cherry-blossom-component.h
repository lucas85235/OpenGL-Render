#pragma once

#include "core/ncsb/component.h"
#include "imp.h"
#include "absl/status/status.h"
#include "native/components/particle_instances/position_utils.h"
#include "native/components/cherry-blossom-effect/cherry_blossom_demo_assets.h"
#include "proto/components/cherry_blossom_state.proto.imp.h"
#include "native/components/particle_instances/particle_instances.h"

using namespace imp;

namespace ix::samsung::homecomponents {
    struct CherryBlossomsData {
    public:
        int amount;
        float radius;
        float max_animation_time;
        float emission_rate;
        float color_blend_amount;
        float size;
        float velocity;
        float3 first_color;
        float3 second_color;
    };

    // Displays the texture of the lighting environment in the background.
    class CherryBlossomComponent : public Component {
    private:
        CherryBlossomState state_;
        NodeHandle particle_instances_node_;
        CherryBlossomsData cherry_blossoms_data_ = {};

        void InitializeParticles();

        void InstanceParticles();

    public:
        using IsfInfo = imp::IsfInfo<&CherryBlossomComponent::state_>;

        absl::Status Setup();

        void Update(const FrameTime &frame_time);

        void RestartStars();
#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };
} // namespace xr::component
