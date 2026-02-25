#include "interactable.h"

namespace ix::samsung::homecomponents {

    void Interactable::Setup(bool can_drag, bool can_click, bool can_double_click, bool can_select) {
        ChangeState_internal(NORMAL);
        interaction_flags_[0] = can_drag;
        interaction_flags_[1] = can_click;
        interaction_flags_[2] = can_double_click;
        interaction_flags_[3] = can_select;
    }

    void Interactable::Update(const FrameTime &frame_Time) {
        time_elapsed_ = frame_Time.GetElapsedSeconds();

        if (current_interaction_state_ == State::HOVER_ENTER || current_interaction_state_ == State::HOVER_MOVED) {
            float3 new_position = MathUtils::MakeVector3FromInfo(event_info_.hit_position);
            HitPositionChanged on_hit_position_changed;
            on_hit_position_changed.position = new_position;
            GetNode()->Send(on_hit_position_changed);
        }

        if (current_interaction_state_ == State::SELECTED) {
            float3 new_position = MathUtils::MakeVector3FromInfo(event_info_.interaction_position);
            InteractionPositionChanged on_position_changed;
            on_position_changed.position = new_position;
            GetNode()->Send(on_position_changed);
            return;
        }

        if (event_info_.interaction_event_type == CLICKSTART) {
            current_button_hold_time_ += frame_Time.GetDeltaSeconds();
            auto delta_position_ = MathUtils::MakeVector3FromInfo(event_info_.interaction_position_delta);

            if (MathUtils::Length(delta_position_) >= movement_delta_threshold_ && interaction_flags_[0]) {
                ChangeState_internal(SELECTED);
            }
        }
    }

    void Interactable::ReceiveInteractionEvent(InteractionEventInfo interaction_info) {
        event_info_ = interaction_info;
        ResetTimers_internal();

        switch (interaction_info.interaction_event_type) {
            case InteractionEventType::HOVERSTART:
                if (current_interaction_state_ != HELD &&
                current_interaction_state_ != SELECTED &&
                !interacting_)
                    ChangeState_internal(State::HOVER_ENTER);
                break;

            case InteractionEventType::HOVERMOVED:
                ChangeState_internal(State::HOVER_MOVED);
                break;

            case InteractionEventType::HOVEREND:
                ChangeState_internal(State::HOVER_EXIT);
                break;

            case InteractionEventType::CLICKSTART:
                if (!interacting_ && time_elapsed_ - last_start_click_time_ < double_click_threshold_ && interaction_flags_[2]) {
                    ChangeState_internal(DOUBLE_CLICKED_DOWN);
                    double_click_started_ = true;
                    interacting_ = true;
                    break;
                }

                if(!interacting_){
                    last_start_click_time_ = time_elapsed_;
                    interacting_ = true;
                    double_click_started_ = false;
                    ChangeState_internal(State::CLICKED_DOWN);
                }
                break;

            case InteractionEventType::CLICKEND:
                if (double_click_started_ && interaction_flags_[2]) {
                    ChangeState_internal(DOUBLE_CLICKED_UP);
                    double_click_started_ = false;
                    break;
                }
                else{
                    if (interaction_flags_[1]) {
                        ChangeState_internal(CLICKED_UP);
                        last_clicked_time_ = time_elapsed_;
                        break;
                    }
                }

                ChangeState_internal(NORMAL);
                interacting_ = false;
                break;
        }
    }

    void Interactable::ChangeState_internal(State new_state) {
        bool update_event = true;
        switch (new_state) {
            case State::HOVER_ENTER:
                if (current_interaction_state_ == HOVER_ENTER || current_interaction_state_ == SELECTED) {
                    update_event = false;
                    break;
                }
                break;

            case State::HOVER_MOVED:
                if (current_interaction_state_ == HOVER_MOVED) {
                    update_event = false;
                    break;
                }
                break;

            case State::HOVER_EXIT:
                if (current_interaction_state_ == HOVER_EXIT) {
                    update_event = false;
                    break;
                }
                break;

            case State::SELECTED:
                if (current_interaction_state_ == SELECTED) {
                    update_event = false;
                    break;
                }
                break;

            case State::NORMAL:
                if (current_interaction_state_ == NORMAL) {
                    update_event = false;
                    break;
                }
                interacting_ = false;
                break;

            case State::CLICKED_DOWN:
                if(current_interaction_state_ == State::CLICKED_DOWN)
                {
                    ChangeState_internal(HELD);
                }
                break;

            case State::CLICKED_UP:
                ChangeState_internal(NORMAL);
                break;


            case State::DOUBLE_CLICKED_DOWN:
                break;

            case State::DOUBLE_CLICKED_UP:
                ChangeState_internal(NORMAL);
                break;

            case State::HELD:
                // do nothing for now, maybe in the future this event can be used
                update_event = false;
                break;
        }

        if (update_event) {
            current_interaction_state_ = new_state;
            InteractionStateChanged OnChangeState;
            OnChangeState.state = current_interaction_state_;
            GetNode()->Send(OnChangeState);
        }
    }

    void Interactable::ResetTimers_internal() {
        current_button_hold_time_ = 0;
    }

    State Interactable::GetInteractionState() {
        return current_interaction_state_;
    }

    void Interactable::SetMovementDeltaThreshold(float new_threshold) {
        movement_delta_threshold_ = new_threshold;
    }
}
