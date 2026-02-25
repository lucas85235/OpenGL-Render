#ifndef COMPONENTS_SPIN_H
#define COMPONENTS_SPIN_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "absl/status/status.h"
#include "proto/components/spin_state.proto.imp.h"

using namespace imp;
namespace ix::samsung::homecomponents
{
// Displays the texture of the lighting environment in the background.
    class Spin : public Component
    {
    private:
        float radians_per_second_;
        SpinState state_;

    public:
        using IsfInfo = imp::IsfInfo<&Spin::state_>;

        absl::Status Setup();
        absl::Status Setup(float velocity);

        void Update(const FrameTime &frame_time);
    };

}  // namespace xr::component

#endif // COMPONENTS_SPIN_H