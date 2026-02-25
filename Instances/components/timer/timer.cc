#include "native/components/timer/timer.h"
#include <memory>
#include <optional>
#include <utility>
#include <filesystem>
#include <tuple>
#include "imp.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void Timer::Update(const FrameTime &frame_time)
    {
        if(!is_counting_)
        {
            return;
        }

        elapsed_ += std::clamp(frame_time.GetDeltaSeconds(), 1.0f / 90.0f, 1.0f / 30.0f);
        if(elapsed_ >= duration_)
        {
            GetNode()->Send(on_timer_completed_);
            is_counting_ = false;
        }
    }

    void Timer::DoTimer(float duration)
    {
        duration_ = duration;
        elapsed_ = 0;
        is_counting_ = true;
    }

    void Timer::StopTimer()
    {
        is_counting_ = false;
        elapsed_ = 0;
    }

    bool Timer::IsTimerActive()
    {
        return false;
    }

    float Timer::GetElapsedTime()
    {
        return elapsed_;
    }

    float Timer::GetPercent()
    {
        if(duration_ > 0) return elapsed_/duration_ <= 1 ? elapsed_/duration_ : 1;
        return 0;
    }
}  // namespace ix::samsung::homecomponents