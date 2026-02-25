#include "native/components/highlight_panel/render/highlight_panel.h"

#include "absl/status/status.h"
#include "core/async/future.h"
#include "core/common/platform_helpers.h"
#include "core/view/framework/render/material.h"
#include "native/components/highlight_panel/data/materials/components_assets.h" //#include "native_soong/data/sysui_assets.h"
#include "proto/components/highlight_visual_state.proto.imp.h"

namespace ix::samsung::homecomponents {

    void HighlightPanel::Setup(imp::float3 position)
    {
        state_.color = imp::float4{ 1.0, 1.0, 1.0, 0.1 };
        state_.border_color = imp::float4{ 1.0, 1.0, 1.0, 0.7 };
        state_.panel_offset_z = imp::float3{ 0.0, 0.0, -0.001} + position;
        state_.render_state = HighlightPanelState::RenderState::NONE;

        state_.boundary_width_meters = 0.0; // todo: remove if not have use
        state_.animation_seconds = 0.3f;
        state_.circle_size = 0.3f;
        state_.is_dinamic_circle_size = true;
        state_.is_resizable = true;

        GetNode()->SetName("HighlightNode");
        GetNode()->SetLocalPosition(state_.panel_offset_z);

        auto on_material_loaded = [=](imp::AssetPtr<imp::MaterialAsset> material) {
            highlight_render_ = GetNode()->AddComponent<imp::RenderComponent>();
            highlight_render_->SetMesh(GetView().GetMeshFactory().CreatePanel(
                    imp::CreateQuadSettings{.size = imp::kOne2 * 2.4}));
            highlight_render_->SetMaterial(GetView().GetMaterialFactory().CreateMaterial(material));
            GetNode()->AddComponent<imp::BoxCollider>(highlight_render_->GetMesh()->GetAabb());

            material_ = highlight_render_->GetMaterial();

            // Setup material parameters
            material_->SetParameter("panelColor", imp::float4{ 0.98, 0.98, 1.0, 0.4 }); // #B8D7FF
            material_->SetParameter("clickedColor", imp::float4{ 0.71, 0.83, 1.0, 0.6 }); // #B8D7FF
            material_->SetParameter("cornerRadius", 0.06f);
            material_->SetParameter("cornerBoundary", 0.35f);
            material_->SetParameter("borderAlpha", 0.8f);
            material_->SetParameter("minMaxBorderThickness", imp::float2{ 0.004, 0.016 });
            material_->SetParameter("minMaxPanelToQuadMargin", imp::float2{ 0.0, 0.05 });
            material_->SetParameter("proportionalCircleSizeFactor", 0.3f);
            material_->SetParameter("useProportionalCircle", 1.0f);
            material_->SetParameter("isResizable", true);

            UpdateSize(GetNode()->GetLocalScale().xy);
        };

        GetView()
                .GetAssetManager()
                .LoadMaterial(assets::kGlowBoundaryCmat)
                .Then(std::move(on_material_loaded))
                .KeptBy(this);

        animator_ = GetNode()->GetOrAddComponent<imp::PropertyAnimator>();
    }

    void HighlightPanel::Update(const imp::FrameTime& frame_time)
    {
        if (!material_)
            return;

        // Always update PanelSize to test in editor mode
        material_->SetParameter("panelSize", GetNode()->GetLocalScale().xy);
        material_->SetParameter("cursorPosition", cursor_position);

        material_->SetParameter("proportionalCircleSizeFactor", state_.circle_size);
        material_->SetParameter("useProportionalCircle", state_.is_dinamic_circle_size ? 1.0f : 0.0f);
        material_->SetParameter("isResizable", state_.is_resizable);

        elapsed_ += frame_time.GetDeltaSeconds();

        // Glow Select Animation
        if (is_selected && (static_cast<int>(state_.render_state <= 1) || !state_.is_resizable)) {
            // This controlEffect returns value between 0 and 1
            float controlEffect = (-cos(elapsed_ * M_PI * 0.5 * 1.5) + 1.0) / 2.0;
            material_->SetParameter("isClicked", true);

            if (controlEffect < 0.99 && isAnimation == true) {
                material_->SetParameter("controlSelectEffect", controlEffect);
            }
            else {
                isAnimation = false;
            }
        }
            // Corner Select Animation
        else if(is_selected && (static_cast<int>(state_.render_state > 1) && state_.is_resizable)) {
            material_->SetParameter("isClicked", true);
        }
        else {
            elapsed_ = 0;
            isAnimation = true;
            material_->SetParameter("isClicked", false);
        }

        material_->SetParameter("changeStateLerp", elapsed_ * 5);
    }

    void HighlightPanel::ChangeHighlightState(bool state) {
        if (is_selected == state)
            return;

        is_selected = state;
    }

    float HighlightPanel::GetHighlightBoundaryWidth() {
        return state_.boundary_width_meters;
    }

    void HighlightPanel::SetIsCornerSelected(bool state) {
        material_->SetParameter("isCornerSelected", state);
    }

    void HighlightPanel::UpdateSize(const imp::float2& new_size_meters) {
        if (!material_) {
            return;
        }

        // todo: Update size in the new shader to fix resize aspect ratio of the object
        material_->SetParameter("panelSize", new_size_meters);
        GetNode()->SetLocalScale(imp::float3{ new_size_meters, 1.0f });
    }

    void HighlightPanel::SetHighlightState(HighlightPanelState::RenderState render_state)
    {
        // TODO(chiantiyan): Update the animation logic when UX provides more detailed specs.
        if (state_.render_state == render_state)
            return;

        state_.render_state = render_state;
        auto change_state = [this, render_state](absl::Status status)
        {
            if (status.ok())
            {
                if (!material_)
                    return;

                // TODO(chiantiyan): revisit later to update shader after render_state is updated.
                material_->SetParameter("state", static_cast<int>(state_.render_state));

                if (render_state != HighlightPanelState::RenderState::NONE && render_state != HighlightPanelState::RenderState::MOVE)
                    FadeIn().KeptBy(this);
            }
        };

        if (imp::AlmostEqual(current_alpha_, 0.0f)) {
            change_state(absl::OkStatus());
        } else {
            FadeOut().Then(std::move(change_state)).KeptBy(this);
        }
    }

    std::unique_ptr<imp::PropertyAnimation> HighlightPanel::CreateFadeAnimation(const float target_alpha)
    {
        const auto sampler = imp::AnimationSampler
                {
                        .times_seconds { .0f, state_.animation_seconds },
                        .values_array = { imp::FloatArray { .values = { current_alpha_, target_alpha } } },
                        .interpolation = imp::INTERPOLATION_LINEAR
                };

        auto set_alpha = [this](const float alpha)
        {
            if (!material_)
                return;

            current_alpha_ = alpha;

            // LerpTime is used on the shader to control variables internally
            material_->SetParameter("lerpTime", current_alpha_);
        };

        return *animator_->AddAnimation(&sampler, std::move(set_alpha));
    }

    imp::Future<absl::Status> HighlightPanel::FadeIn() {
        if (fadein_animation_) {
            fadein_animation_->Stop();
            fadein_animation_ = {};
        }

        // current_alpha_ = 0.0;
        fadein_animation_ = CreateFadeAnimation(1.0f);
        return fadein_animation_->PlayAsync();
    }

    imp::Future<absl::Status> HighlightPanel::FadeOut() {
        if (fadeout_animation_) {
            fadeout_animation_->Stop();
            fadeout_animation_ = {};
        }

        // current_alpha_ = 1.0;
        fadeout_animation_ = CreateFadeAnimation(0.0f);
        return fadeout_animation_->PlayAsync();
    }

} // namespace ix::samsung::homecomponents