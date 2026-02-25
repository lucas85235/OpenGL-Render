#ifndef COMPONENTS_TIMER_H
#define COMPONENTS_TIMER_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "absl/status/status.h"

namespace ix::samsung::homecomponents
{
    struct TimerCompleted : imp::Event
    {
        int placeholder = 0;
    };

    class Timer : public imp::Component
    {
    private:
        float duration_;
        float elapsed_;
        bool is_counting_;
        TimerCompleted on_timer_completed_;

    public:
        void Update(const imp::FrameTime &frame_time);
        void DoTimer(float duration);
        void StopTimer();
        float GetElapsedTime();
        float GetPercent();
        bool IsTimerActive();
    };

}  // namespace ix::samsung::homecomponents

#endif // COMPONENTS_TIMER_H