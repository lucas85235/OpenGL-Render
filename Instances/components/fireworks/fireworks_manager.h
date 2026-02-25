#ifndef COMPONENTS_FIREWORKSMANAGER_H
#define COMPONENTS_FIREWORKSMANAGER_H

#include <vector>
#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/fireworks_state.proto.imp.h"
#include "native/components/fireworks/fireworks.h"

namespace ix::samsung::homecomponents
{
    struct FireworksInstanceData
    {
        imp::float3 position;
        std::string name;
        imp::float2 play_interval_range;
        FireworksState fireworks_state;
        float play_interval;
        float elapsed;
        imp::ComponentHandle<Fireworks> component;
    };

    class FireworksManager : public imp::Component
    {
    private:
        FireworksManagerState state_;
        std::vector<FireworksInstanceData> fireworks_data_;
        imp::NodeHandle root_node_;

        void AddFireworksNode(FireworksInstanceData data);
        void Initialize();
        float RandomNumber(float min, float max);

    public:
        void Setup();
        void Update(const imp::FrameTime& frame_time);
        using IsfInfo = imp::IsfInfo<&FireworksManager::state_>;

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };
}

#endif // COMPONENTS_FIREWORKSMANAGER_H