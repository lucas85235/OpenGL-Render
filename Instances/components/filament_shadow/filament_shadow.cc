#include "filament_shadow.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void FilamentShadow::Setup(bool enabled)
    {
        Component::Setup();

        view_ = GetView().GetHost()->GetView();

        state_.enable_shadow = enabled;
        UpdateShadowOption();
    }

    void FilamentShadow::UpdateShadowOption() {
        view_->setShadowingEnabled(state_.enable_shadow);
    }

#if IMP_RUNTIME(DEV)
    // Customize Editor UI for this component
    void FilamentShadow::DrawEditorUi()
    {
        if (ImGui::Button("Update shadow")) {
            UpdateShadowOption();
        }
    }
#endif // IMP_RUNTIME(DEV)
}
