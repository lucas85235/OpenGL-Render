#ifndef COMPONENTS_FILAMENT_SHADOW_H
#define COMPONENTS_FILAMENT_SHADOW_H

#include "imp.h"
#include "proto/components/filament_shadow_state.proto.imp.h"

namespace ix::samsung::homecomponents
{
    class FilamentShadow : public imp::Component
    {
    private:
        FilamentShadowState state_;
        filament::View *view_{};
    public:
        void Setup(bool enabled);
        void UpdateShadowOption();
        using IsfInfo = imp::IsfInfo<&FilamentShadow::state_>;

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };
}  // namespace ix::samsung::homecomponents
#endif // COMPONENTS_FILAMENT_SHADOW_H
