#include "native/components/hand/hand.h"

#include <cstring>
#include <string>

#include "core/common/debug_draw.h"
#include "core/math/math.h"
#include "core/view/framework/render/render_component.h"
#include "proto/components/hand_renderer_state.proto.imp.h"
#include "native/openxr/hand_tracking/handtracking_actions.h"
#include "native/openxr/hand_tracking/hand_actions.h"

#if IMP_PLATFORM(ANDROID)
#include "native/openxr/hand_tracking/handtracking_actions.h"
#endif

namespace ix::samsung::homecomponents {

void Hand::Setup(HandRenderer::Handedness handType) {

    handType_ = handType;
    hand_renderer_ = GetNode()->AddComponent<HandRenderer>(handType_);
}

void Hand::Update(const imp::FrameTime& deltaTime) {
    if(hand_renderer_){
        UpdateHandActions();
    }
}

void Hand::UpdateHandActions() {
    // Recover hand tracking instance
    const OpenXrHandtrackingActions& handtracking_actions =
            *GetView().GetRegistry().Get<OpenXrHandtrackingActions>();

    // Identify handedness
    const HandActions& hand = (handType_ == HandRenderer::Handedness::LEFT)
            ? handtracking_actions.left_hand_actions()
            : handtracking_actions.right_hand_actions();

    if (!hand.active()) {
        return;
    }

    hand_renderer_->UpdateHandJoints(hand);
}

} // namespace ix::samsung::homecomponents
