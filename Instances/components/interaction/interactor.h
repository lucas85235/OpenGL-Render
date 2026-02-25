#ifndef HOME_COMPONENTS_INTERACTOR_H
#define HOME_COMPONENTS_INTERACTOR_H

#include "proto/components/interaction_state.proto.imp.h"
#include "core/input/pointer_event.h"
#include "core/ncsb/component.h"

using namespace imp;
namespace ix::samsung::homecomponents {
    class Interactor{

    protected:
        virtual void BindEvents() = 0;
    };
}

#endif //HOME_COMPONENTS_INTERACTOR_H
