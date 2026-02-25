#include "native/components/follow/follow.h"
#include "imp.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void Follow::Setup(NodeHandle target_anchor, float delay, bool keep_position, NodeHandle look_at)
    {
        if(look_at.IsValid())
        {
            billboard_ = true;
            look_at_ = look_at;
        }
        else
        {
            billboard_ = false;
        }

        keep_position_ = keep_position;
        owner_ = GetNode();
        delay_ = clamp(delay, 0.1f, 1.0f);

        // Create an anchor for obtaining the target position/rotation to follow
        anchor_ = GetView().CreateNode();
        anchor_->SetWorldPosition(owner_->GetWorldPosition());

        if (keep_position_)
        {
            // Create an axis that will ever rotate to look target object
            axis_ = GetView().CreateNode();
            axis_->SetWorldPosition(target_anchor->GetWorldPosition());
            // Attach anchor to the axis node and axis to the target
            axis_->SetParentKeepWorldTransform(target_anchor);
            anchor_->SetParentKeepWorldTransform(axis_);
            camera_node_ = GetView().GetCameraManager().GetCamera()->GetNode();
        }
        else
        {
            // Attach anchor to the followed node
            anchor_->SetParentKeepWorldTransform(target_anchor);
        }
    }

    void Follow::Update(const FrameTime &frame_time)
    {
        // Use a lerp to add a delay to the position change
        //auto target_position = float3( anchor_->GetWorldPosition().x, owner_->GetWorldPosition().y, anchor_->GetWorldPosition().z);

        float3 new_position = lerp(owner_->GetWorldPosition(),
                                   anchor_->GetWorldPosition(), delay_);

        // Update the position and rotation of this node
        owner_->SetWorldPosition(new_position);

        if(keep_position_)
        {
            auto up_local_vector = GetUpVector(axis_);
            auto transform = Transform<float>(mat4f::lookAt(camera_node_->GetWorldPosition(), axis_->GetWorldPosition(), up_local_vector));
            axis_->SetWorldRotation(transform.rotation);
        }

        if (billboard_)
        {
            auto up_local_vector = GetUpVector(owner_);
            auto transform = Transform<float>(mat4f::lookAt(look_at_->GetWorldPosition(), owner_->GetWorldPosition(), up_local_vector));
            owner_->SetWorldRotation(transform.rotation);
        }
        else
        {
            owner_->SetWorldRotation(anchor_->GetWorldRotation());
        }
    }

    float3 Follow::GetUpVector(NodeHandle node)
    {
        auto rotation = node->GetWorldRotation();
        float pitch = rotation.x;
        float yaw = rotation.y;
        float3 up(0,0,0);
        up.x = sin(pitch) * sin(yaw);
        up.y = cos(pitch);
        up.z = sin(pitch) * cos(yaw);
        return up;
    }
}  // namespace xr::component