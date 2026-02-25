#include "native/components/highlight_panel/render/highlight_visual.h"

#include "absl/status/status.h"
#include "core/async/future.h"
#include "core/common/platform_helpers.h"
#include "core/view/framework/render/material.h"
#include "native/components/highlight_panel/data/materials/components_assets.h" //#include "native_soong/data/sysui_assets.h"
#include "proto/components/highlight_visual_state.proto.imp.h"

namespace ix::samsung::homecomponents {

void HighlightVisual::Setup(const imp::float2& panel_size_meters) {
    state_.animation_seconds = 0.2f;
    state_.color = imp::float4{1.0, 1.0, 1.0, 0.1};
    state_.border_color = imp::float4{1.0, 1.0, 1.0, 0.7};
    state_.panel_offset_z = imp::float3{0.0, 0.0, -0.001};
    state_.render_state = HighlightVisualState::RenderState::NONE;
    // TODO(chiantiyan): Get content panel's corner radius from proto buffer and let the highlight's
    // corner radius be panel radius + boundary size.
    state_.corner_radius_meters = 0.08f;
    state_.boundary_width_meters = 0.1f;
    state_.border_width_meters = 0.01f;
    state_.border_smoothing_percentage = 0.2f;

    GetNode()->SetName("HighlightNode");
    GetNode()->SetLocalPosition(state_.panel_offset_z);

    auto on_material_loaded = [this,
                               panel_size_meters](imp::AssetPtr<imp::MaterialAsset> material) {
        highlight_render_ = GetNode()->AddComponent<imp::RenderComponent>();
        highlight_render_->SetMesh(GetView().GetMeshFactory().CreatePanel(
                imp::CreateQuadSettings{.size = imp::kOne2}));
        highlight_render_->SetMaterial(GetView().GetMaterialFactory().CreateMaterial(material));
        GetNode()->AddComponent<imp::BoxCollider>(highlight_render_->GetMesh()->GetAabb());

        material_ = highlight_render_->GetMaterial();
        UpdateSize(panel_size_meters);
    };

    GetView()
            .GetAssetManager()
            .LoadMaterial(assets::kGlowBoundaryCmat)
            .Then(std::move(on_material_loaded))
            .KeptBy(this);

    animator_ = GetNode()->GetOrAddComponent<imp::PropertyAnimator>();
}

void HighlightVisual::Update(const imp::FrameTime& frame_time) {
    if (!material_) {
        return;
    }

    material_->SetParameter("cornerBoundary", state_.boundary_width_meters * 2.0f);
    material_->SetParameter("cornerRadius", state_.corner_radius_meters);
    material_->SetParameter("color", state_.color);
    material_->SetParameter("borderColor", state_.border_color);
    material_->SetParameter("borderWidth", state_.border_width_meters);

    const float smoothing_distance_meters =
            state_.border_smoothing_percentage * state_.border_width_meters;
    material_->SetParameter("smoothingDistance", smoothing_distance_meters);
}

float HighlightVisual::GetHighlightBoundaryWidth() {
    return state_.boundary_width_meters;
}

void HighlightVisual::UpdateSize(const imp::float2& new_size_meters) {
    if (!material_) {
        return;
    }

    const imp::float2 highlight_size_meters = new_size_meters + 2.0f * state_.boundary_width_meters;
    material_->SetParameter("size", highlight_size_meters);
    GetNode()->SetLocalScale(imp::float3{highlight_size_meters, 1.0f});
}

void HighlightVisual::SetHighlightState(HighlightVisualState::RenderState render_state) {
    // TODO(chiantiyan): Update the animation logic when UX provides more detailed specs.
    if (state_.render_state == render_state) {
        return;
    }
    state_.render_state = render_state;

    auto change_state = [this, render_state](absl::Status status) {
        if (status.ok()) {
            if (!material_) {
                return;
            }
            // TODO(chiantiyan): revisit later to update shader after render_state is updated.
            material_->SetParameter("state", static_cast<int>(state_.render_state));

            if (render_state != HighlightVisualState::RenderState::NONE) {
                FadeIn().KeptBy(this);
            }
        }
    };

    if (imp::AlmostEqual(current_alpha_, 0.0f)) {
        change_state(absl::OkStatus());
    } else {
        FadeOut().Then(std::move(change_state)).KeptBy(this);
    }
}

std::unique_ptr<imp::PropertyAnimation> HighlightVisual::CreateFadeAnimation(
        const float target_alpha) {
    const auto sampler = imp::AnimationSampler{.times_seconds{.0f, state_.animation_seconds},
                                               .values_array = {imp::FloatArray{
                                                       .values = {current_alpha_, target_alpha}}},
                                               .interpolation = imp::INTERPOLATION_LINEAR};
    auto set_alpha = [this](const float alpha) {
        if (!material_) {
            return;
        }

        current_alpha_ = alpha;
        highlight_render_->SetEnabled(!imp::AlmostEqual(current_alpha_, 0.0f));
        material_->SetParameter("fadeAlpha", current_alpha_);
    };

    return *animator_->AddAnimation(&sampler, std::move(set_alpha));
}

imp::Future<absl::Status> HighlightVisual::FadeIn() {
    if (fadein_animation_) {
        fadein_animation_->Stop();
        fadein_animation_ = {};
    }

    fadein_animation_ = CreateFadeAnimation(1.0f);
    return fadein_animation_->PlayAsync();
}

imp::Future<absl::Status> HighlightVisual::FadeOut() {
    if (fadeout_animation_) {
        fadeout_animation_->Stop();
        fadeout_animation_ = {};
    }

    fadeout_animation_ = CreateFadeAnimation(0.0f);
    return fadeout_animation_->PlayAsync();
}

} // namespace ix::samsung::homecomponents
