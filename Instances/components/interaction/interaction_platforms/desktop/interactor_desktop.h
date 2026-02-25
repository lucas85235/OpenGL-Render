#ifndef HOME_COMPONENTS_INTERACTOR_DESKTOP_H
#define HOME_COMPONENTS_INTERACTOR_DESKTOP_H

#include "imp.h"
#include "core/view/base_view.h"
#include "native/components/interaction/interactor.h"
#include "native/home_components_utils/common/math/math.h"
#include "native/components/interaction/interactor_factory.h"
#include "native/components/interaction/interactable.h"
#include "core/view/framework/assets/material_factory.h"
#include "core/view/framework/render/render_component.h"
#include "native/home_components_utils/common/math/math.h"
#include "native/home_components_utils/common/shapes/shapes.h"


using namespace imp;
namespace ix::samsung::homecomponents {
    class InteractorDesktop : public Interactor, public Component{
    public:
        void Setup();

    private:
        void BindEvents() override;
        NodeHandle owner_;
        NodeHandle sphere_;
        NodeHandle camera_node_;
        NodeHandle last_hovered_object_;
        NodeHandle interaction_knob_node_;
        float3 last_interaction_position_;
        float interaction_distance_ = 2;
        float scroll_sensibility_ = 0.001f;
        InteractionEventInfo last_interaction_info_;
        InteractionEventType current_interaction_state_ = InteractionEventType::HOVEREND;
        ComponentHandle<Interactable> last_hovered_component_;
        bool interacting_with_object_;
        const PointerEventType button_down_ = PointerEventType::kDown;
        const PointerEventType button_up_ = PointerEventType::kUp;
        InteractionEventInfo GetControllerInteractionPosition(const PointerHitEvent &event);
        InteractionEventInfo GetControllerHitPosition(const PointerHitEvent &event);
        float3 PointToWorldPosition(float2 point);
        void UpdateInteractionKnobPosition(const PointerHitEvent &event);
        void CreateInteractionKnobNode();
    };
}

#endif //HOME_COMPONENTS_INTERACTOR_DESKTOP_H
