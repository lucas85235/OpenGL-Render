#ifndef HOME_COMPONENTS_INTERACTOR_ANDROID_H
#define HOME_COMPONENTS_INTERACTOR_ANDROID_H

#include "imp.h"
#include "native/components/interaction/interactor.h"
#include "native/components/interaction/interactor_factory.h"

using namespace imp;
namespace ix::samsung::homecomponents {
    class InteractorAndroid : public Interactor, public Component{
    public:
        void Setup();

    private:
        void BindEvents() override;
        NodeHandle owner_;
        NodeHandle last_hovered_object_;
        NodeHandle camera_node_;
        InteractionEventType current_interaction_state_ = InteractionEventType::HOVEREND;
        ComponentHandle<Interactable> last_hovered_component_;
        float interaction_distance_ = 2;
        bool interacting_with_object_;
        const PointerEventType button_down_ = PointerEventType::kDown;
        const PointerEventType button_up_ = PointerEventType::kUp;
        InteractionEventInfo GetControllerInteractionPosition(const PointerHitEvent &event);
    };
}

#endif //HOME_COMPONENTS_INTERACTOR_ANDROID_H
