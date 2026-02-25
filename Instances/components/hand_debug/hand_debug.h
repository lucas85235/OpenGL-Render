#ifndef COMPONENTS_HAND_DEBUG_H
#define COMPONENTS_HAND_DEBUG_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "absl/status/status.h"
#include "core/view/platforms/xr_android/xr_action_controller.h"
#include "core/actions/controller_events.h"


using namespace imp;

namespace ix::samsung::homecomponents
{
    class HandDebug : public Component
    {
    public:
        void Setup();
        void BindEvents();
        void ChangeControllerModelPosition(const ControllerHitEvent &event);
    };
}

#endif // COMPONENTS_HAND_DEBUG_H
