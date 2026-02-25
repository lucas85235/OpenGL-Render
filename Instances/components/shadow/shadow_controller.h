#ifndef COMPONENTS_SHADOW_H
#define COMPONENTS_SHADOW_H

#include "imp.h"
#include "core/ncsb/component.h"
#include <string>

using namespace imp;
using namespace std;

namespace ix::samsung::homecomponents
{
    class ShadowController : public Component
    {
        private:
            NodeHandle target_node_;
            std::string detect_raycast_ = "DetectShadowRaycast";
            void OnEnable();
            void OnDisable();
            float3 hit_normal_;

        public:
            void Setup(NodeHandle target_node);
            void Update(const FrameTime& frame_time);
            void SetActive(bool active);
    };
}

#endif // COMPONENTS_SHADOW_H
