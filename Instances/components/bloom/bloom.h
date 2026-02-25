#ifndef COMPONENTS_BLOOM_H
#define COMPONENTS_BLOOM_H

#include "imp.h"
#include "proto/components/bloom_state.proto.imp.h"

namespace ix::samsung::homecomponents
{
    class Bloom : public imp::Component
    {
    private:
        BloomState state_;
        filament::View *view_{};

    public:
        void Setup();

        void Setup(bool enabled, float strength, bool threshold);

        void UpdateEffect();

        using IsfInfo = imp::IsfInfo<&Bloom::state_>;
#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };
} // namespace ix::samsung::homecomponents

#endif // COMPONENTS_BLOOM_H
