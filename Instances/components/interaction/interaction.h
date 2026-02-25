#ifndef COMPONENTS_INTERACTION_H
#define COMPONENTS_INTERACTION_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "absl/status/status.h"
#include "core/math/math.h"
#include "absl/strings/str_format.h"
#include "proto/components/interaction_state.proto.imp.h"
#include "native/components/interaction/interactable.h"

#if IMP_PLATFORM(ANDROID)
#include "native/data/home_components_assets.h"
#include "core/view/platforms/xr_android/xr_action_controller.h"
#include "core/view/platforms/xr_android/xr_opengl_platform.h"
#include "core/xr/openxr_events.h"
#include "core/actions/input_action_event.h"
#include "core/actions/controller_events.h"
#include "core/actions/action_config.h"
#include "core/actions/controller_input_handler.h"
#endif

using namespace imp;
namespace ix::samsung::homecomponents
{



}  //namespace ix::samsung::homecomponents

#endif // COMPONENTS_INTERACTION_H