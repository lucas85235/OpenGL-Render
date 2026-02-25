#ifndef COMPONENTS_TRAIL_H
#define COMPONENTS_TRAIL_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/trail_state.proto.imp.h"
#include "native/components/particle_instances/particle_instances.h"
#include "native/components/trail/trail_assets.h"

namespace ix::samsung::homecomponents
{
    struct TrailData {
    public:
        bool play;
    };

    class Trail : public imp::Component
    {
    private:
        TrailState state_;
        TrailData trailData_;
        NodeHandle particle_instance_node_, particle_instance_child_node_;
        MaterialPtr trail_material;
        Material* tr_material_;

    public:
        void Setup();
        void Update(const imp::FrameTime& frame_time);
        void InitializeTrail();
        void InstanceTrail(TrailData td_);

        using IsfInfo = imp::IsfInfo<&Trail::state_>;
    };
}

#endif // COMPONENTS_TRAIL_H