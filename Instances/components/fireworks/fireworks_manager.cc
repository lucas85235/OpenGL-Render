#include <random>

#include "core/common/log.h"
#include "imp.h"
#include "fireworks_manager.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void FireworksManager::Setup() 
    {
        Initialize();
    }

    void FireworksManager::Update(const FrameTime& frame_time)
    {
        for (FireworksInstanceData& data : fireworks_data_)
        {
            if (data.elapsed > data.play_interval)
            {
                data.component->Play();
                data.elapsed = 0;
                data.play_interval = RandomNumber(data.play_interval_range.x, data.play_interval_range.y);
            }
            data.elapsed += frame_time.GetDeltaSeconds();
        }
    }

    void FireworksManager::Initialize() {
        root_node_ = GetView().CreateNode();
        root_node_->SetName("Fireworks root node");
        root_node_->SetParent(GetNode());

        fireworks_data_.clear();

        for (const FireworksInstanceState& data : state_.fireworkInstances) {
            auto [name, position, play_interval_range_x, play_interval_range_y, fireworks_state] = data;
            AddFireworksNode({
                position,
                name,
                { play_interval_range_x, play_interval_range_y },
                fireworks_state
            });
        }
    }

    void FireworksManager::AddFireworksNode(FireworksInstanceData data)
    {
        const NodeHandle node = GetView().CreateNode();
        node->SetName(data.name);
        node->SetParent(root_node_);
        node->SetLocalPosition(data.position);

        data.elapsed = 0;
        data.play_interval = RandomNumber(data.play_interval_range.x, data.play_interval_range.y);
        data.component = node->AddComponentWithState<Fireworks>(data.fireworks_state);

        fireworks_data_.push_back(data);
    }

    float FireworksManager::RandomNumber(float min, float max)
    {
        auto generator = std::mt19937(rand());
        std::uniform_real_distribution<float> dist(min, max);
        return dist(generator);
    }

#if IMP_RUNTIME(DEV)
    // Customize Editor UI for this component
    void FireworksManager::DrawEditorUi()
    {
       if (ImGui::Button("Reload effect"))
       {
           GetView().DestroyNode(root_node_);
           Initialize();
       }
    }
#endif
}
