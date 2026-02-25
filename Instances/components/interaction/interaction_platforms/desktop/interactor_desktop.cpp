#include "interactor_desktop.h"
namespace ix::samsung::homecomponents {

    void InteractorDesktop::Setup() {
        Component::Setup();
        owner_ = GetNode();
        camera_node_ = GetView().GetCameraManager().GetCamera()->GetNode();
        last_hovered_object_ = NodeHandle();
        CreateInteractionKnobNode();
        BindEvents();
    }

    void InteractorDesktop::BindEvents() {
        GetView().GetDispatcher().Connect([this](const PointerHitEvent &event) mutable {
            InteractionEventInfo new_interaction_event;
            UpdateInteractionKnobPosition(event);
            //updates interaction position
            if(interacting_with_object_ && last_hovered_object_.IsValid()){
                new_interaction_event = GetControllerInteractionPosition(event);
                last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                last_interaction_info_ = new_interaction_event;
            }

            NodeHandle current_object;
            bool hover_enabled = !interacting_with_object_ && event.GetHitNode().IsValid() &&
                    event.GetHitNode()->GetComponent<Interactable>().IsValid();

            bool hover_moved_status = current_interaction_state_ == InteractionEventType::HOVERSTART
                                   || current_interaction_state_ == InteractionEventType::HOVERMOVED;

            if (hover_enabled && !interacting_with_object_ && last_hovered_object_.IsValid() && hover_moved_status){
                last_hovered_component_ = last_hovered_object_->GetComponent<Interactable>();
                if (last_hovered_component_.IsValid()) {
                    new_interaction_event = GetControllerHitPosition(event);
                    new_interaction_event.interaction_event_type = InteractionEventType::HOVERMOVED;
                    last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                    last_interaction_info_ = new_interaction_event;
                    current_interaction_state_ = new_interaction_event.interaction_event_type;
                }
            }

            //start hover
            if(hover_enabled && !interacting_with_object_){
                current_object = event.GetHitNode();
                if(current_interaction_state_ != HOVERSTART && current_interaction_state_ != HOVERMOVED){
                    last_hovered_object_ = current_object;
                    last_hovered_component_ = last_hovered_object_->GetComponent<Interactable>();
                    if (last_hovered_component_.IsValid()) {
                        new_interaction_event.interaction_event_type = InteractionEventType::HOVERSTART;
                        last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                        last_interaction_info_ = new_interaction_event;
                        current_interaction_state_ = new_interaction_event.interaction_event_type;
                    }
                }
                //if the interactable changes, reset the previous component state and assign the new one
                else if(last_hovered_object_.IsValid() && last_hovered_object_ != current_object){
                    new_interaction_event.interaction_event_type = InteractionEventType::HOVEREND;
                    last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                    last_hovered_object_ = current_object;
                    last_hovered_component_ = last_hovered_object_->GetComponent<Interactable>();
                    if (last_hovered_component_.IsValid()) {
                        new_interaction_event.interaction_event_type = InteractionEventType::HOVERSTART;
                        last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                        last_interaction_info_ = new_interaction_event;
                    }
                }
            }

            //end hover and reset variables
            if(!interacting_with_object_ && !event.GetHitNode().IsValid() && current_interaction_state_ != HOVEREND){
                if(last_hovered_object_.IsValid() && last_hovered_component_.IsValid()){
                    new_interaction_event.interaction_event_type = InteractionEventType::HOVEREND;
                    current_interaction_state_ = new_interaction_event.interaction_event_type;
                    last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                    last_interaction_info_ = new_interaction_event;
                    last_hovered_object_ = NodeHandle();
                    last_hovered_component_ = ComponentHandle<Interactable>();
                }
            }

            auto eventType = event.event.Type();

            //start click
            if(current_object.IsValid() && hover_moved_status && eventType == button_down_){
                current_interaction_state_ = CLICKSTART;
                float3 camera_position = camera_node_->GetWorldPosition();
                if(last_hovered_object_.IsValid()) interaction_distance_ = MathUtils::GetDistance(camera_position, last_hovered_object_->GetWorldPosition());
                new_interaction_event = GetControllerInteractionPosition(event);
                if(last_hovered_object_.IsValid()) last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                last_interaction_info_ = new_interaction_event;
                interacting_with_object_ = true;
            }

            //end click
            if(interacting_with_object_ && current_interaction_state_ == CLICKSTART && eventType == button_up_){
                current_interaction_state_ = CLICKEND;
                new_interaction_event.interaction_event_type = current_interaction_state_;
                interacting_with_object_ = false;
                if(last_hovered_object_.IsValid()) last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                last_interaction_info_ = new_interaction_event;

                if (event.GetHitNode().IsValid() && last_hovered_object_.IsValid()) {
                    last_hovered_object_ = event.GetHitNode();
                    last_hovered_component_ = last_hovered_object_->GetComponent<Interactable>();
                    if (last_hovered_component_.IsValid()) {
                        new_interaction_event.interaction_event_type = InteractionEventType::HOVERSTART;
                        last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                        last_interaction_info_ = new_interaction_event;
                        current_interaction_state_ = new_interaction_event.interaction_event_type;
                    }
                }
            }
        }, this);

        //changes the interaction distance based on the amount of scrolling actions
        GetView().GetDispatcher().Connect([this](const WheelScrollEvent &event) mutable {
            if(!last_hovered_component_.IsValid()) return;
            interaction_distance_ = interaction_distance_ + event.event.GetDelta() * scroll_sensibility_;
            InteractionEventInfo new_interaction_event = last_interaction_info_;
            auto new_interaction_position = MathUtils::MakeInfoFromVector3(float3(new_interaction_event.interaction_position.x,new_interaction_event.interaction_position.y,-interaction_distance_));
            new_interaction_event.interaction_position = new_interaction_position;
            last_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
        }, this);
    }

    InteractionEventInfo InteractorDesktop::GetControllerInteractionPosition(const PointerHitEvent &event) {
        auto pointer = event.event.GetPointer();
        InteractionEventInfo new_interaction_event;
        float3 delta_point_position = float3(pointer.delta.x, pointer.delta.y, 0);
        new_interaction_event.interaction_event_type = current_interaction_state_;
        new_interaction_event.interaction_position_delta = MathUtils::MakeInfoFromVector3(delta_point_position);
        new_interaction_event.interaction_position = PointToWorldPosition(pointer.point);
        return new_interaction_event;
    }

    InteractionEventInfo InteractorDesktop::GetControllerHitPosition(const PointerHitEvent &event) {
        auto pointer = event.event.GetPointer();
        InteractionEventInfo new_interaction_event;
        new_interaction_event.interaction_event_type = current_interaction_state_;
        new_interaction_event.hit_position = PointToWorldPosition(pointer.point);
        return new_interaction_event;
    }

    float3 InteractorDesktop::PointToWorldPosition(float2 point) {
        float2 pointer_screen_position = point;
        float3 pointer_world_position = MathUtils::ScreenToWorldPoint(pointer_screen_position, GetView(), interaction_distance_);
        float3 converted_position = MathUtils::MakeInfoFromVector3(pointer_world_position);
        return converted_position;
    }

    void InteractorDesktop::UpdateInteractionKnobPosition(const PointerHitEvent &event) {
        auto pointer = event.event.GetPointer();
        float2 pointer_screen_position = pointer.point;
        float3 pointer_world_position = MathUtils::ScreenToWorldPoint(pointer_screen_position, GetView(), 0.3);
        auto collision_manager = CollisionManager(&GetView());
        Ray world_ray = Ray(camera_node_->GetWorldPosition(), pointer_world_position);
        auto hits = collision_manager.IntersectAll(world_ray);

        if(hits.size() <= 0){
            interaction_knob_node_->SetEnabled(false);
            return;
        }
        else{
            if(!interaction_knob_node_->IsEnabled())
                interaction_knob_node_->SetEnabled(true);
        }

        interaction_knob_node_->SetWorldPosition(hits[0].world_point);
    }

    void InteractorDesktop::CreateInteractionKnobNode() {
        interaction_knob_node_ = GetView().CreateNode();
        interaction_knob_node_->SetName("Interaction knob");

        ShapeUtils::CreateSphereShape(interaction_knob_node_, 0.005f, data::kInteractionMaterialCmat, 7);
    }
}
