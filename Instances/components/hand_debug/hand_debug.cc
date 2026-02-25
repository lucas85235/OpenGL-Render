#include "imp.h"
#include "native/components/hand_debug/hand_debug.h"
#include "core/common/debug_draw.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void HandDebug::Setup() {
        BindEvents();
    }

    void HandDebug::BindEvents() {
        if (GetView().GetRegistry().Get<XrActionController>().ok()) {
            GetView().GetDispatcher().Connect([this](const ControllerHitEvent &event) mutable {
                if (event.GetHand() != ControllerHitEvent::Hand::kRight) return;
                ChangeControllerModelPosition(event);
            }, this);            
        }
    }

    void HandDebug::ChangeControllerModelPosition(const ControllerHitEvent &event) {
        if (GetView().GetRegistry().Get<XrActionController>().ok()) {
            imp::float3 ball_position = event.GetControllerTransform().translation;
            debug_draw::Global().SphereLines(ball_position, 0.025f, debug_draw::GetColor(debug_draw::DebugColor::kRed));
            debug_draw::Global().Line(ball_position, ball_position + event.GetControllerRay().direction * 1.5f, debug_draw::GetColor(debug_draw::DebugColor::kRed));
        }
    }
}