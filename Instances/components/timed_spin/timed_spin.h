#ifndef COMPONENTS_TIMEDSPIN_H
#define COMPONENTS_TIMEDSPIN_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "absl/status/status.h"

namespace ix::samsung::homecomponents
{
    class TimedSpin : public imp::Component
    {
    private:
        float duration_;
        float elapsed_;
        bool is_spinning_;
        imp::quatf initial_rotation_;
        imp::quatf final_rotation_;

    public:
        void Update(const imp::FrameTime &frame_time);
        void DoSpin(imp::float3 axis, float duration, float degrees);
    };

}  // namespace ix::samsung::homecomponents

#endif // COMPONENTS_TIMEDSPIN_H