#pragma once

#include "imp.h"

#include "core/animation/property_animator.h"
#include "core/ncsb/node_handle.h"
#include "proto/components/highlight_visual_state.proto.imp.h"

namespace ix::samsung::homecomponents {

// Component that renders the move and resize boundary around the panel.
class HighlightPanel : public imp::Component {
public:
    // Give the parameter a defualt value so it can be registered with the SceneSystem.
    void Setup(imp::float3 position);
    void Update(const imp::FrameTime& frame_time);

    // Setting a new RenderState will fade out the current RenderState and fade in the new one. If
    // the new RenderState is NONE and we skip the fade-in. If the highlight is currently disabled,
    // we skip the fade-out. This function can safely be called mid fade animations.
    void SetHighlightState(HighlightPanelState::RenderState render_state);
    void SetIsCornerSelected(bool state);
    void ChangeHighlightState(bool state);

    float GetHighlightBoundaryWidth();

    void UpdateSize(const imp::float2& new_size_meters);

private:
    imp::Future<absl::Status> FadeIn();
    imp::Future<absl::Status> FadeOut();
    std::unique_ptr<imp::PropertyAnimation> CreateFadeAnimation(float target_alpha);

    imp::ComponentHandle<imp::RenderComponent> highlight_render_;
    imp::Material* material_;
    float elapsed_ = 0.0f;
    bool isAnimation = false;
    bool is_selected = false;
    bool last_selected_state = false;

    imp::ComponentHandle<imp::PropertyAnimator> animator_;
    std::unique_ptr<imp::PropertyAnimation> fadeout_animation_;
    std::unique_ptr<imp::PropertyAnimation> fadein_animation_;

    HighlightPanelState state_;

public:
    using IsfInfo = imp::IsfInfo<&HighlightPanel::state_>;
    imp::float2 cursor_position;
    float current_alpha_ = 0.0f;
};

} // namespace ix::samsung::homecomponents
