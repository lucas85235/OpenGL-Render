#ifndef HOME_COMPONENTS_INTERACTOR_FACTORY_H
#define HOME_COMPONENTS_INTERACTOR_FACTORY_H

#include "imp.h"
#include "core/ncsb/component.h"
#include "proto/components/interaction_state.proto.imp.h"
#include "native/components/interaction/interaction.h"
#include "native/components/interaction/interaction_platforms/desktop/interactor_desktop.h"
#include "native/components/interaction/interactor.h"

#if IMP_PLATFORM(ANDROID)
#include "native/components/interaction/interaction_platforms/xr/interactor_xr.h"
#include "native/components/interaction/interaction_platforms/android/interactor_android.h"
#endif

using namespace imp;

namespace ix::samsung::homecomponents {
    class InteractorFactory {
    public:
        static void CreateInteractor(NodeHandle interactor_node);
    };
}


#endif //HOME_COMPONENTS_INTERACTOR_FACTORY_H
