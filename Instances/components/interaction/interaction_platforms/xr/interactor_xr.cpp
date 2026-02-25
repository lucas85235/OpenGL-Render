#include "interactor_xr.h"

#if IMP_RUNTIME(DEV)
#include "core/common/debug_draw.h"
#endif

namespace ix::samsung::homecomponents {

    void InteractorXR::Setup() {
        owner_ = GetNode();
        current_hovered_object_ = NodeHandle();
        camera_node_ = GetView().GetCameraManager().GetCamera()->GetNode();
        CreateInteractionKnobNode();
        SetupControllerModel("Controller Right");
        BindEvents();
    }

    void InteractorXR::BindEvents() {
        GetView().GetDispatcher().Connect([this](const ControllerHitEvent &event) mutable {
            if (!controller_model_.IsValid()) return;
            if (event.GetHand() != ControllerHitEvent::Hand::kRight) return;

            ChangeControllerModelPosition(event);

            //TODO: uncomment this line when the left controller is working
            //if (active_hand_.has_value() && event.GetHand() != active_hand_) return;

            InputActionState<bool> select_button_state = event.GetInputActionState<bool>(imp::kDefaultSelectActionName);
            bool button_has_changed_state = select_button_state.has_changed_since_last_sync;
            select_button_down_ = select_button_state.current_state;
            InteractionEventInfo new_interaction_event;
            //updates interaction position
            if (interacting_with_object_ && current_hovered_object_.IsValid()) {
                new_interaction_event = GetControllerInteractionPosition(event);
                current_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
            }

            NodeHandle current_object = event.GetHitNode();
            bool hover_enabled = !interacting_with_object_ && event.GetHitNode().IsValid() && event.GetHitNode()->GetComponent<Interactable>().IsValid();

            bool hover_moved_status = current_interaction_state_ == InteractionEventType::HOVERSTART
                                      || current_interaction_state_ == InteractionEventType::HOVERMOVED;

            if (hover_enabled && current_hovered_object_.IsValid() && hover_moved_status){
                new_interaction_event = GetControllerHitPosition(event);
                new_interaction_event.interaction_event_type = InteractionEventType::HOVERMOVED;
                current_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
            }

            //start hover
            if (hover_enabled  && current_interaction_state_ != InteractionEventType::HOVERSTART && current_interaction_state_ != InteractionEventType::HOVERMOVED) {
                //if the interactable changes, reset the previous component state and assign the new one. Sends a hover event to the new one
                if (current_object.IsValid() && current_hovered_object_.IsValid() && current_hovered_object_ != current_object && current_hovered_object_->GetComponent<Interactable>().IsValid()) {
                    new_interaction_event.interaction_event_type = InteractionEventType::HOVEREND;
                    current_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                    current_hovered_object_ = current_object;
                    current_hovered_component_ = current_hovered_object_->GetComponent<Interactable>();
                    if (current_hovered_component_.IsValid()) {
                        new_interaction_event.interaction_event_type = InteractionEventType::HOVERSTART;
                        current_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                    }
                } else if (current_object.IsValid() && current_interaction_state_ != HOVERSTART && current_object->GetComponent<Interactable>().IsValid()) {
                    current_hovered_object_ = current_object;
                    current_hovered_component_ = current_hovered_object_->GetComponent<Interactable>();
                    new_interaction_event.interaction_event_type = InteractionEventType::HOVERSTART;

                    if (interaction_knob_node_.IsValid() && !interaction_knob_node_->IsEnabled())
                        interaction_knob_node_->SetEnabled(true);

                    ChangeInteractionLinesColor(hovered_color_);

                    if (current_hovered_component_.IsValid())
                        current_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                    current_interaction_state_ = new_interaction_event.interaction_event_type;
                }
            }

            auto hit_distance = event.GetHit()->distance;

            //Set the interaction line node scale based on interaction distance
            if (current_object.IsValid() && hit_distance > 0.1f) {
                line_parent_->SetLocalScale(float3(1, 1, hit_distance));

            } else {
                line_parent_->SetLocalScale(float3(1, 1, default_interaction_line_distance_));
            }

            //end hover and reset variables
            if ((!current_object.IsValid() || current_hovered_object_ != current_object) && !interacting_with_object_ && current_interaction_state_ != HOVEREND) {
                if (current_hovered_object_.IsValid()) {
                    new_interaction_event.interaction_event_type = InteractionEventType::HOVEREND;

                    if (interaction_knob_node_.IsValid() && interaction_knob_node_->IsEnabled())
                        interaction_knob_node_->SetEnabled(false);

                    ChangeInteractionLinesColor(normal_color_);
                    current_interaction_state_ = new_interaction_event.interaction_event_type;
                    current_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                    current_hovered_object_ = NodeHandle();
                    current_hovered_component_ = ComponentHandle<Interactable>();
                }
            }

            //start click
            if (current_object.IsValid() && hover_moved_status && select_button_down_ && button_has_changed_state) {
                auto hit_distance = event.GetHit()->distance;
                current_interaction_state_ = CLICKSTART;
                ChangeInteractionLinesColor(interaction_color_);
                interaction_distance_ = hit_distance;

                auto box_collider = current_object->GetComponent<BoxCollider>();

                if (box_collider.IsValid()) {
                    auto box = box_collider->GetWorldBox();
                    interaction_distance_offset_ = box.halfExtent.z;
                }

                if (current_hovered_component_.IsValid()) {
                    current_hovered_component_->SetMovementDeltaThreshold(0.015f);
                    new_interaction_event = GetControllerInteractionPosition(event);
                    current_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
                    interacting_with_object_ = true;
                }
            }

            //end click
            if (interacting_with_object_ && current_interaction_state_ == CLICKSTART && !select_button_down_) {
                current_interaction_state_ = CLICKEND;
                ChangeInteractionLinesColor(normal_color_);
                new_interaction_event.interaction_event_type = current_interaction_state_;
                interacting_with_object_ = false;
                current_hovered_component_->ReceiveInteractionEvent(new_interaction_event);
            }

            controller_interaction_position_delta_ = MathUtils::MakeVector3FromInfo(GetControllerInteractionPosition(event).interaction_position) - last_controller_interaction_position_;
            last_controller_interaction_position_ = MathUtils::MakeVector3FromInfo(GetControllerInteractionPosition(event).interaction_position);
            UpdateInteractionKnobPosition(event.GetHit()->world_point);

        }, this);

    }

    InteractionEventInfo InteractorXR::GetControllerInteractionPosition(const ControllerHitEvent &event) {
        InteractionEventInfo new_interaction_event;

        //controller delta position used to handle drag events
        float3 delta_controller_position = controller_interaction_position_delta_;
        float3 converted_delta_position = MathUtils::MakeInfoFromVector3(delta_controller_position);

        new_interaction_event.interaction_position_delta = converted_delta_position;
        new_interaction_event.interaction_event_type = current_interaction_state_;
        new_interaction_event.interaction_position = ControllerRayToWorldPosition(event);

        return new_interaction_event;
    }

    InteractionEventInfo InteractorXR::GetControllerHitPosition(const ControllerHitEvent &event) {
        InteractionEventInfo new_interaction_event;
        new_interaction_event.interaction_event_type = current_interaction_state_;
        new_interaction_event.hit_position = ControllerRayToWorldPosition(event);

        return new_interaction_event;
    }
    float3 InteractorXR::ControllerRayToWorldPosition(const ControllerHitEvent &event) {
        //get`s the controller position
        auto current_controller_translation = event.GetControllerTransform().translation;

        //get`s the controller ray
        Ray controller_ray = event.GetControllerRay();

        float3 interaction_position = current_controller_translation + controller_ray.direction * interaction_distance_;
        interaction_position -= float3(0, 0, interaction_distance_offset_);

        //calculate the final position based on interaction distance and controller`s position
        float3 converted_position = MathUtils::MakeInfoFromVector3(interaction_position);

        return converted_position;
    }

    void InteractorXR::ChangeControllerModelPosition(const ControllerHitEvent &event) {

        //Set controller model transform
        auto controller_transform = event.GetControllerTransform();
        auto controller_position = controller_transform.translation;
        controller_parent_->SetWorldRotation(event.GetControllerTransform().rotation);
        controller_parent_->SetWorldPosition(controller_position);
    }

    void InteractorXR::SetupControllerModel(std::string controller_name) {
        controller_parent_ = GetView().CreateNode();
        controller_parent_->SetName("Controller parent");
        controller_ = GetView().CreateNode();
        controller_->SetParent(controller_parent_);
        controller_->SetName(controller_name);

        //creates the line node that's contains both interaction lines.
        //This node is used mainly to scale the lines to respond to the interaction distance variation
        line_parent_ = GetView().CreateNode();
        line_parent_->SetName("Line Origin");

        controller_parent_->SetWorldPosition(float3(0, 0, 0));

        CreateControllerModel();
        SetupInteractionLines();
    }

    // Loads the controller model
    void InteractorXR::CreateControllerModel() {
        GetView().GetAssetManager().LoadModel(assets::kControllerRightGlb)
            .Then([=](NodeHandle controller_node) {
                controller_model_ = controller_node;
                controller_model_->GetChildren()[0]->GetChildren()[0]->GetComponent<GltfCollider>()->SetEnabled(false);
                controller_model_->GetChildren()[0]->GetChildren()[0]->GetComponent<GltfCollider>()->Cleanup();
                controller_model_->SetName("ControllerModel");
                controller_model_->SetParent(controller_);
                line_parent_->SetParent(controller_model_);
                line_parent_->SetLocalRotation(QuatFromEuler(float3(45, 0, 0)));
                controller_model_->SetLocalRotation(QuatFromEuler(float3(-45, 180, 0)));
            }).KeptBy(this);
    }

    Future<ComponentHandle<LineRenderer>> InteractorXR::CreateInteractionLine(const float3 start, const float3 end, float3 face_vector, float line_tickness, float line_feather, float4 color) {
        LineRendererState line_definition;
        NodeHandle line_node = GetView().CreateNode();
        line_definition.points = {start, end};
        line_definition.normal = face_vector;
        line_definition.color = color;
        line_definition.width = line_tickness;
        line_definition.feather = line_feather;
        line_definition.wrap = false;
        line_definition.end_cap_shape = LineCapShape::LINE_CAP_SHAPE_ROUNDED;
        line_definition.start_cap_shape = LineCapShape::LINE_CAP_SHAPE_ROUNDED;
        auto line_renderer_component = line_node->AddComponentWithState<LineRenderer>(line_definition);
        return line_renderer_component;
    }

    void InteractorXR::ChangeInteractionLinesColor(TVec4<float> &color) {
        if (!face_right_line_.IsValid() || !face_up_line_.IsValid()) return;
        face_right_line_->SetColor(color);
        face_up_line_->SetColor(color);
    }

    void InteractorXR::SetupInteractionLines() {
        NodeHandle face_right_line_node = GetView().CreateNode();
        NodeHandle face_up_line_node = GetView().CreateNode();
        NodeHandle interaction_knob = GetView().CreateNode();

        float distance_ = 1;

        auto controller_model_position = float3(0);
        auto controller_model_forward = controller_->GetWorldForward();
        auto controller_line_end = (controller_model_position - controller_model_forward);
        auto face_right_vector = float3(1, 0, 0);
        auto face_up_vector = float3(0, 1, 0);
        auto line_color = float4(1);
        auto line_thickness = 0.005;
        auto line_feather = 1;

        //Create controller interaction right face line
        auto line_component_right = CreateInteractionLine(controller_model_position, controller_line_end, face_right_vector, line_thickness, line_feather, line_color);
        line_component_right.Then([=](ComponentHandle<LineRenderer> line_renderer) {
            face_right_line_node->SetParent(line_parent_);
            face_right_line_node->SetName("Vertical Line");
            face_right_line_ = line_renderer;
            face_right_line_->GetNode()->SetParent(face_right_line_node);
        }).KeptBy(this);

        //Create controller interaction up face line
        auto line_component_up = CreateInteractionLine(controller_model_position, controller_line_end, face_up_vector, line_thickness, line_feather, line_color);
        line_component_up.Then([=](ComponentHandle<LineRenderer> line_renderer) {
            face_up_line_node->SetParent(line_parent_);
            face_up_line_node->SetName("Horizontal Line");
            face_up_line_ = line_renderer;
            face_up_line_->GetNode()->SetParent(face_up_line_node);
        }).KeptBy(this);
    }

    void InteractorXR::UpdateInteractionKnobPosition(const float3 position) {
        //if(!interaction_knob_node_.IsValid()) return;
        interaction_knob_node_->SetWorldPosition(position);
    }

    void InteractorXR::CreateInteractionKnobNode() {
        interaction_knob_node_ = GetView().CreateNode();
        interaction_knob_node_->SetName("Interaction knob");
        ShapeUtils::CreateSphereShape(interaction_knob_node_, 0.01f, data::kInteractionMaterialCmat, 7);
    }
}