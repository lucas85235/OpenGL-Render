#include "native/components/spin/spin.h"
#include <memory>
#include <optional>
#include <utility>
#include <filesystem>
#include <tuple>
#include "imp.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    absl::Status Spin::Setup()
    {
        return absl::OkStatus();
    }

    absl::Status Spin::Setup(float velocity)
    {
        state_.degrees_per_second = velocity;
        return absl::OkStatus();
    }

    void Spin::Update(const FrameTime &frame_time)
    {
        // Updates the rotation based on the time passed since the previous frame.
        radians_per_second_ = ToRadians(state_.degrees_per_second);
        float delta_radians = radians_per_second_ * frame_time.GetDeltaSeconds();
        quatf delta_rotation = quatf::fromAxisAngle(kUp, delta_radians);
        GetNode()->SetLocalRotation(GetNode()->GetLocalRotation() * delta_rotation);
    }
}  // namespace xr::component