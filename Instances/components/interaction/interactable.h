#ifndef HOME_COMPONENTS_INTERACTABLE_H
#define HOME_COMPONENTS_INTERACTABLE_H

#include "imp.h"
#include "core/ncsb/dispatcher/event.h"
#include "native/home_components_utils/common/math/math.h"
#include "core/math/math.h"
#include "proto/components/interaction_state.proto.imp.h"

#if IMP_RUNTIME(DEV)
#include "core/common/debug_draw.h"
#endif

using namespace imp;

namespace ix::samsung::homecomponents{

    struct InteractionEvent : Event{
        InteractionEventInfo interaction_event;
    };

    struct InteractionStateChanged : Event{
        State state;
    };

    struct InteractionPositionChanged : Event{
        float3 position;
    };

    struct HitPositionChanged : Event{
        float3 position;
    };

    class Interactable : public Component {
    public:
        void ReceiveInteractionEvent(InteractionEventInfo interaction_info);
        void Setup(bool can_drag, bool can_click, bool can_double_click, bool can_select);
        void Update(const FrameTime& frame_Time);
        State GetInteractionState();
        void SetMovementDeltaThreshold(float new_threshold);

    private:
        State current_interaction_state_;
        void ChangeState_internal(State new_state);
        void ResetTimers_internal();
        float last_clicked_time_ = 0;
        bool interacting_ = false;
        float last_start_click_time_ = 0;
        float double_click_threshold_ = 0.3;
        float time_elapsed_ = 0;
        float movement_delta_threshold_ = 0.03;
        float current_button_hold_time_ = 0;
        bool double_click_started_ = false;

        //0: Can drag
        //1: Can click
        //2: Can double click
        //3: Can select
        std::bitset<4> interaction_flags_;
        InteractionEventInfo event_info_;
    };
}

#endif //HOME_COMPONENTS_INTERACTABLE_H
