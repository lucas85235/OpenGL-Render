#ifndef HOME_COMPONENTS_INTERACTOR_XR_H
#define HOME_COMPONENTS_INTERACTOR_XR_H

#include "imp.h"
#include "native/components/interaction/interactor_factory.h"
#include "native/components/interaction/interactor.h"
#include "core/line_renderer/line_renderer.h"
#include "native/home_components_utils/common/math/math.h"
#include "native/home_components_utils/common/shapes/shapes.h"

using namespace imp;
namespace ix::samsung::homecomponents {
    class InteractorXR : public Interactor, public Component {

    public:
        void Setup();

    private:
        NodeHandle owner_;
        NodeHandle controller_model_;
        NodeHandle controller_;
        NodeHandle controller_parent_;
        NodeHandle line_parent_;
        NodeHandle current_hovered_object_;
        NodeHandle interaction_knob_node_;
        NodeHandle camera_node_;
        InteractionEventType current_interaction_state_ = InteractionEventType::HOVEREND;
        ComponentHandle<RenderComponent> interaction_knob_render_component_;
        ComponentHandle<Interactable> current_hovered_component_;
        float3 last_controller_interaction_position_;
        float3 controller_interaction_position_delta_;
        float3 hit_normal_surface_;
        ComponentHandle<LineRenderer> face_up_line_;
        ComponentHandle<LineRenderer> face_right_line_;
        ComponentHandle<LineRenderer> interaction_knob_line_component_;
        bool select_button_down_;
        bool interacting_with_object_;
        float interaction_distance_ = 3.0f;
        float interaction_distance_offset_ = 0;
        float default_interaction_line_distance_ = 0.5f;
        TVec4<float> interaction_color_ = TVec4<float>(0,0.1,0.7,1);
        TVec4<float> normal_color_ = TVec4<float>(1,1,1,1);
        TVec4<float> hovered_color_ = TVec4<float>(0,0.7,0,1);
        TVec4<float> debug_color_ = TVec4<float>(1,0,0,1);

        //function that's verifies if this platform has controllers
        absl::optional<ControllerHitEvent::Hand> active_hand_;

        InteractionEventInfo GetControllerInteractionPosition(const ControllerHitEvent &event);
        InteractionEventInfo GetControllerHitPosition(const ControllerHitEvent &event);
        float3 ControllerRayToWorldPosition(const ControllerHitEvent &event);

        void BindEvents() override;

        void ChangeControllerModelPosition(const ControllerHitEvent &event);

        void UpdateInteractionKnobRotation();

        void UpdateInteractionKnobPosition(const float3 position);

        void SetupControllerModel(std::string controller_name);

        void SetupInteractionLines();

        void ChangeInteractionLinesColor(TVec4<float>& color);

        Future<ComponentHandle<LineRenderer>> CreateInteractionLine(const float3 start, const float3 end, float3 face_vector, float line_tickness, float line_feather, float4 color);

        void CreateControllerModel();

        void CreateInteractionKnobNode();
    };
}

#endif //HOME_COMPONENTS_INTERACTOR_XR_H
