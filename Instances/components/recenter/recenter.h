#ifndef COMPONENTS_RECENTER_H
#define COMPONENTS_RECENTER_H

#include "core/ncsb/component.h"
#include "imp.h"

#if IMP_PLATFORM(ANDROID)
    #include "core/xr/openxr_events.h"
    #include "core/view/platforms/xr_android/xr_session_host.h"
    #include "third_party/OpenXR_KHR/generated/include/openxr/openxr.h"
#endif

namespace ix::samsung::homecomponents
{
    class Recenter : public imp::Component
    {
    public:
        void Setup();
        void CreateSpaceReference();

        imp::Dispatcher::ScopedConnection begin_session_connection_;
        imp::Dispatcher::ScopedConnection space_change_pending_connection_;
#if IMP_PLATFORM(ANDROID)
        XrSpace space_;
#endif
    };
}
#endif // COMPONENTS_RECENTER_H
