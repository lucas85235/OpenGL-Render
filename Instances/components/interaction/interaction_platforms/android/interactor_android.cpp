#include "interactor_android.h"

namespace ix::samsung::homecomponents {

    void InteractorAndroid::Setup() {
        Component::Setup();
        owner_ = GetNode();
        camera_node_ = GetView().GetCameraManager().GetCamera()->GetNode();
        last_hovered_object_ = NodeHandle();
        BindEvents();
    }

    void InteractorAndroid::BindEvents() {
        GetView().GetDispatcher().Connect([this](const PointerHitEvent &event) mutable {
            InteractionEventInfo new_interaction_event;
            //updates interaction position
            if(interacting_with_object_ && last_hovered_object_.IsValid()){
                new_interaction_event = GetControllerInteractionPosition(event);
                last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
            }


            NodeHandle current_object;
            bool hover_enabled = !interacting_with_object_ && event.GetHitNode().IsValid() &&
                                 event.GetHitNode()->GetComponent<Interactable>().IsValid();

            //start hover
            if(hover_enabled && !interacting_with_object_){
                current_object = event.GetHitNode();
                if(current_interaction_state_ != HOVERSTART){
                    last_hovered_object_ = current_object;
                    float3 camera_position = camera_node_->GetWorldPosition();
                    interaction_distance_ = MathUtils::GetDistance(camera_position, last_hovered_object_->GetWorldPosition());
                    last_hovered_component_ = last_hovered_object_->GetComponent<Interactable>();
                    new_interaction_event.interaction_event_type = InteractionEventType::HOVERSTART;
                    last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                    current_interaction_state_ = new_interaction_event.interaction_event_type;
                }
                    //if the interactable changes, reset the previous component state and assign the new one
                else if(last_hovered_object_.IsValid() && last_hovered_object_ != current_object){
                    new_interaction_event.interaction_event_type = InteractionEventType::HOVEREND;
                    last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                    last_hovered_object_ = current_object;
                    last_hovered_component_ = last_hovered_object_->GetComponent<Interactable>();
                    new_interaction_event.interaction_event_type = InteractionEventType::HOVERSTART;
                    last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                }
            }

            //end hover and reset variables
            if(!interacting_with_object_ && !event.GetHitNode().IsValid() && current_interaction_state_ != HOVEREND){
                if(last_hovered_object_.IsValid()){
                    new_interaction_event.interaction_event_type = InteractionEventType::HOVEREND;
                    current_interaction_state_ = new_interaction_event.interaction_event_type;
                    last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                    last_hovered_object_ = NodeHandle();
                    last_hovered_component_ = ComponentHandle<Interactable>();
                }
            }

            auto eventType = event.event.Type();

            //start click
            if(current_object.IsValid() && current_interaction_state_ == HOVERSTART && eventType == button_down_){
                current_interaction_state_ = CLICKSTART;
                new_interaction_event = GetControllerInteractionPosition(event);
                if(last_hovered_object_.IsValid()) last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                interacting_with_object_ = true;
            }

            //end click
            if(interacting_with_object_ && current_interaction_state_ == CLICKSTART && eventType == button_up_){
                current_interaction_state_ = CLICKEND;
                new_interaction_event.interaction_event_type = current_interaction_state_;
                interacting_with_object_ = false;
                if(last_hovered_object_.IsValid()) last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
            }
        }, this);

    }

    InteractionEventInfo InteractorAndroid::GetControllerInteractionPosition(const PointerHitEvent &event) {
        auto pointer = event.event.GetPointer();
        InteractionEventInfo new_interaction_event;
        float3 delta_point_position = float3(pointer.delta.x, pointer.delta.y, 0);
        float2 pointer_screen_position = pointer.point;
        float3 pointer_world_position = MathUtils::ScreenToWorldPoint(pointer_screen_position, GetView(), interaction_distance_);
        float3 converted_position = MathUtils::MakeInfoFromVector3(pointer_world_position);
        new_interaction_event.interaction_event_type = current_interaction_state_;
        new_interaction_event.interaction_position_delta = MathUtils::MakeInfoFromVector3(delta_point_position);
        new_interaction_event.interaction_position = converted_position;
        return new_interaction_event;
    }
}
