#include "native/components/shadow/shadow_controller.h"
#include "native/components/tag/tag.h"
#include "imp.h"
#include <cmath>
#include <string>

using namespace imp;
using namespace std;

namespace ix::samsung::homecomponents
{
    void ShadowController::Setup(NodeHandle target_node)
    {
        target_node_ = target_node;
    }

    void ShadowController::Update(const FrameTime& frame_time)
    {
        auto object_position = GetNode()->GetWorldPosition();

        Ray world_ray = Ray(object_position, kDown);
        float3 direction_debug = world_ray.GetPointAt(1);

        auto collision_manager = CollisionManager(&GetView());
        auto hits = collision_manager.IntersectAll(world_ray, Flags<CollisionMask>(CollisionMask::kAll));

        RayHit hit;
        bool find_hit = false;

        for (size_t i = 0; i < hits.size(); i++)
        {
            if (hits[i].node != target_node_ && hits[i].node != GetNode())
            {
                auto tag_component = hits[i].node->GetComponent<Tag>();
                if (tag_component && tag_component->Compare(detect_raycast_))
                {
                    find_hit = true;
                    hit = hits[i];

                    // Set shadow decal position and rotation
                    target_node_->SetWorldPosition(hit.world_point + (kUp * 0.001f));

                    target_node_->SetWorldRotation(QuatFromEuler(float3(90,0,0)));
                    break;
                }
            }
        }

        target_node_->SetEnabled(find_hit);
    }

    void ShadowController::SetActive(bool active)
    {
        if (this->IsEnabled() != active)
        {
            this->SetEnabled(active);
            active ? OnEnable() : OnDisable();
        }
    }

    void ShadowController::OnEnable()
    {
        target_node_->SetEnabled(true);
    }

    void ShadowController::OnDisable()
    {
        target_node_->SetEnabled(false);
    }
}