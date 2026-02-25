#ifndef COMPONENTS_FIREWORKS_H
#define COMPONENTS_FIREWORKS_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/fireworks_state.proto.imp.h"

namespace ix::samsung::homecomponents
{
    class Fireworks : public imp::Component
    {
    private:
        FireworksState state_;
        void Initialize();
        imp::NodeHandle fireworks_node_;

    public:
        void Setup();
        void Update(const imp::FrameTime& frame_time);
        using IsfInfo = imp::IsfInfo<&Fireworks::state_>;

        void Play();

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };
}

#endif // COMPONENTS_FIREWORKS_H