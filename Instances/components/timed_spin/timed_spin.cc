#include "native/components/timed_spin/timed_spin.h"
#include <memory>
#include <optional>
#include <utility>
#include <filesystem>
#include <tuple>
#include "imp.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void TimedSpin::Update(const FrameTime &frame_time)
    {
        if(!is_spinning_)
        {
            return;
        }

        // When DoSpin() is called, at each frame set the local rotation to the spherical linear interpolation 
        // (a.k.a. slerp) of the two rotations
        GetNode()->SetLocalRotation(slerp(initial_rotation_, final_rotation_, elapsed_ / duration_));
        elapsed_ += frame_time.GetDeltaSeconds();

        if(elapsed_ >= duration_)
        {
            // When the time is up, directly set the final rotation to avoid small deviations on the angle
            GetNode()->SetWorldRotation(final_rotation_);
            is_spinning_ = false;
        }
    }

    void TimedSpin::DoSpin(float3 axis, float duration, float degrees)
    {
        if(is_spinning_)
        {
            output::Warning("%s: DoSpin called when there is already a spin being done!");
            return;
        }
        duration_ = duration;
        initial_rotation_ = GetNode()->GetLocalRotation();
        // Final rotation is calculated adding the final rotation value to the initial rotation
        // With quaternions, this is done via multiplication
        final_rotation_ = 
            initial_rotation_ * quatf::fromAxisAngle(axis, ToRadians(degrees));
        elapsed_ = 0;
        is_spinning_ = true;
    }
}  // namespace ix::samsung::homecomponents