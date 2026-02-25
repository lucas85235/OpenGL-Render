#ifndef HOME_COMPONENTS_SURFACE_BASE_H
#define HOME_COMPONENTS_SURFACE_BASE_H

#include "core/ncsb/component.h"
#include "core/math/math.h"
#include "imp.h"
#include "absl/status/status.h"
#include "native/home_components_utils/common/shapes/shapes.h"
#include "native/home_components_utils/common/math/math.h"
#include "native/data/home_components_assets.h"
#include "native/components/tag/tag.h"
#include "core/geometry/closest_point.h"
#include "native/home_components_common.h"
#include "native/components/surface_base/surface_assets.h"
#include "native/home_components_utils/common/event/event.h"
#include "samples/building_home/components/animated_piece/animated_piece.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    class SurfaceBase : public Component
    {

    public:
        const float3 surface_color_ = COLOR_BLUE;
        const float surface_alpha_ = 0.5f;
        void Setup();
        void Cleanup();
        void Update(const FrameTime &frame_time);
        void FindFloor();
        utils::Event<bool, AnimatedPiece> OnEnableEvent;
        AnimatedPiece piece_animation;

    private:
        NodeHandle surface_mesh_;
        NodeHandle owner_node_;
        float max_height_ = 0.1f;
        float cur_height_ = 0.f;
        float3 cur_base_scale_ = float3(0);
        float3 model_offset_;
        void LoadSurfaceAssets();
        void FindCurrentHeightAndFloor();
        int current_state_ = -1;
    };
}  // namespace xr::component

#endif // HOME_COMPONENTS_SURFACE_BASE_H