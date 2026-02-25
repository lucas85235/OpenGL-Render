#include "imp.h"
#include "core/common/log.h"
#include "recenter.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void Recenter::Setup()
    {
        #if IMP_PLATFORM(ANDROID)
            begin_session_connection_ = GetView().GetDispatcher().Connect(
                [this](const imp::OpenXrReferenceSpaceCreatedEvent& result)
                {
                    imp::XrSessionHost& sessionHost = static_cast<imp::XrSessionHost&>(*GetView().GetHost());
                    this->space_ = sessionHost.GetXrSpace();
                }
            );

            space_change_pending_connection_ = GetView().GetDispatcher().Connect(
                [this](const imp::OpenXrSpaceChangePendingEvent& result)
                {
                    CreateSpaceReference();
                }
            );
        #endif
    }

    void Recenter::CreateSpaceReference()
    {
        #if IMP_PLATFORM(ANDROID)
        xrDestroySpace(this->space_);

        const XrReferenceSpaceCreateInfo create_info = {
            .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
            .next = nullptr,
            .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR,
            .poseInReferenceSpace = {{0.f, 0.f, 0.f, 1.f}, {0.f}},
        };

        imp::XrSessionHost& sessionHost = static_cast<imp::XrSessionHost&>(*GetView().GetHost());

        XrResult result = xrCreateReferenceSpace(sessionHost.GetXrSession(), &create_info, &this->space_);
        #endif
    }
}
