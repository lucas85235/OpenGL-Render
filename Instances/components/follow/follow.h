#ifndef COMPONENTS_FOLLOW_H
#define COMPONENTS_FOLLOW_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "absl/status/status.h"

using namespace imp;
namespace ix::samsung::homecomponents
{
// Follows the determined node.
    class Follow : public Component
    {
    private:
        float delay_;
        bool billboard_;
        bool keep_position_;
        NodeHandle look_at_;
        NodeHandle anchor_;
        NodeHandle axis_;
        NodeHandle owner_;
        NodeHandle camera_node_;
        float3 GetUpVector(NodeHandle node);
    public:
        void Setup(NodeHandle target_anchor, float delay, bool keep_position, NodeHandle look_at = NodeHandle());
        void Update(const FrameTime &frame_time);
    };

}  // namespace xr::component

#endif // COMPONENTS_FOLLOW_H