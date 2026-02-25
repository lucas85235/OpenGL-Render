#include "bloom.h"

#if IMP_RUNTIME(DEV)
#include "dear_imgui/imgui.h"
#endif

namespace ix::samsung::homecomponents
{
    void Bloom::Setup()
    {
        Component::Setup();

        view_ = GetView().GetHost()->GetView();
    }

    void Bloom::Setup(bool enabled, float strength, bool threshold)
    {
        view_ = GetView().GetHost()->GetView();

        state_.effect_enabled = enabled;
        state_.strength = strength;
        state_.threshold = threshold;

        UpdateEffect();
    }

    void Bloom::UpdateEffect()
    {

        filament::View::BloomOptions bloom_options;
        bloom_options.enabled = state_.effect_enabled;
        bloom_options.strength = state_.strength;
        bloom_options.threshold = state_.threshold;

        view_->setBloomOptions(bloom_options);
    }

#if IMP_RUNTIME(DEV)
    // Customize Editor UI for this component
    void Bloom::DrawEditorUi()
    {
        if (ImGui::Button("Set Bloom Options")) {
            UpdateEffect();
        }
    }
#endif // IMP_RUNTIME(DEV)
} // namespace ix::samsung::homecomponents
