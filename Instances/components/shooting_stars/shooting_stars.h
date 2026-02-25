#pragma once

#include "core/ncsb/component.h"
#include "imp.h"
#include "native/home_components_utils/common/math/math.h"
#include "third_party/filament/filament/include/filament/Texture.h"
#include "proto/components/shooting_stars_state.proto.imp.h"

using namespace std;

namespace ix::moohan::home_support {

    class ShootingStars : public imp::Component {

    private:

        ShootingStarsState state_;
        imp::AssetPtr<imp::MaterialAsset> shooting_stars_material_asset_;
        imp::Material *shooting_stars_material_ptr_;
        imp::ComponentHandle<imp::RenderComponent> renderer_;
        bool animation_in_progress_ = false;
        imp::float2 random_float_x_;
        imp::float2 random_float_y_;
        imp::float2 random_float_z_;
        NodeHandle camera_node_;
        NodeHandle owner_;

        float count_time_ = 0.0f;
        float delta_time_ = 0.0f;
        bool can_animate = true;
        bool initialized = false;

        void InProgressAnimation(float delta_seconds);
        void StopAnimation();
        void ResetAnimation();
        void ApplyDelayedRandomPosition();
        void ChangePanelSize(float panel_size);

        float Lerp(float a, float b, float f);
        float Random01();
        float RandomValues(float a, float b);
        float GetRandom(float min, float max);
        void LookAtCamera();

    public:
        using IsfInfo = imp::IsfInfo<&ShootingStars::state_,
                imp::IsfDependencies<imp::RenderComponent>>;
        void Setup(imp::resources::ResourceDefinition asset);
        void Update(const imp::FrameTime& frame_time);
        void OnIsfStateChanged();
    };

}  // namespace ix::moohan::home_support
