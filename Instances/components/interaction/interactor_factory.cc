#include "interactor_factory.h"

namespace ix::samsung::homecomponents {

    void InteractorFactory::CreateInteractor(imp::NodeHandle interactor_node) {
#if IMP_PLATFORM(ANDROID)
        if (interactor_node->GetView().GetRegistry().Get<XrActionController>().ok()) {
            interactor_node->SetName("Interactor XR");
            interactor_node->AddComponent<InteractorXR>().Get();
        }
        else{
            interactor_node->SetName("Interactor Android");
            interactor_node->AddComponent<InteractorAndroid>().Get();
        }

#elif IMP_PLATFORM(DESKTOP)
        interactor_node->SetName("Interactor Desktop");
        interactor_node->AddComponent<InteractorDesktop>().Get();
#endif
    }
}
