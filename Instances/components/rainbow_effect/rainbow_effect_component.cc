#include "rainbow_effect_component.h"

#include "imp.h"
#include "core/common/log.h"
#include "native/components/rainbow_effect/rainbow_effect_component.h"
#include "native/components/rainbow_effect/rainbow_effect_assets.h"

using namespace imp;

namespace ix::samsung::homecomponents {
    void RainbowEffectComponent::Setup() {
        InitializeRainbowEffect();
    }

    void RainbowEffectComponent::InitializeRainbowEffect() {
        rainbowEffectData_.rainbowRingsThickness = state_.rainbowRingsThickness.value_or(
            rainbowEffectData_.rainbowRingsThickness);
        rainbowEffectData_.rainbowRingsSmoothness = state_.rainbowRingsSmoothness.value_or(
            rainbowEffectData_.rainbowRingsSmoothness);
        rainbowEffectData_.rainbowRadiusSmoothness = state_.rainbowRadiusSmoothness.value_or(
            rainbowEffectData_.rainbowRadiusSmoothness);
        rainbowEffectData_.rainbowStartRenderingAngle = state_.rainbowStartRenderingAngle.value_or(
            rainbowEffectData_.rainbowStartRenderingAngle);
        rainbowEffectData_.rainbowEndRenderingAngle = state_.rainbowEndRenderingAngle.value_or(
            rainbowEffectData_.rainbowEndRenderingAngle);
        rainbowEffectData_.fadeStartAngle = state_.fadeStartAngle.value_or(
            rainbowEffectData_.fadeStartAngle);
        rainbowEffectData_.fadeEndAngle = state_.fadeEndAngle.value_or(
            rainbowEffectData_.fadeEndAngle);
        rainbowEffectData_.colorIntensity = state_.colorIntensity.value_or(rainbowEffectData_.colorIntensity);
        rainbowEffectData_.opacity = state_.opacity.value_or(rainbowEffectData_.opacity);
        rainbowEffectData_.position = state_.position.value_or(rainbowEffectData_.position);
        rainbowEffectData_.rotation = state_.rotation.value_or(rainbowEffectData_.rotation);
        rainbowEffectData_.scale = state_.scale.value_or(rainbowEffectData_.scale);

        InstanceRainbowEffect(rainbowEffectData_);
    }

    void RainbowEffectComponent::InstanceRainbowEffect(RainbowEffectData rbw_) {
        auto rainbow_effect_mat_future = GetView().GetAssetManager().LoadMaterial(assets::kRainbowEffectCmat);
        auto rainbow_effect_texture_future = GetView().GetAssetManager().LoadImage(assets::kTXRainbow2048Png);

        rainbow_effect_mat_future.Merge(rainbow_effect_texture_future)
                .Then([=](std::tuple<AssetPtr<MaterialAsset>, AssetPtr<ImageAsset> > result) {
                    auto [material, texture] = result;

                    node = GetView().CreateNode();
                    node->SetName("rainbow_effect");
                    node->SetParent(GetNode());

                    node->SetLocalPosition(rainbowEffectData_.position);
                    node->SetLocalRotation(QuatFromEuler(rainbowEffectData_.rotation));
                    node->SetLocalScale(rainbowEffectData_.scale);

                    ComponentHandle<RenderComponent> renderer_model = node->AddComponent<RenderComponent>(
                        RenderComponent::FrustrumCullingMode::kDisabled);

                    auto meshQuad = GetView().GetMeshFactory().CreateQuad({
                        .size = imp::float2(1.0f)
                    });

                    renderer_model->SetMesh(std::move(meshQuad));

                    auto material_ptr_ = GetView().GetMaterialFactory().CreateMaterial(material);

                    imp::TextureFactory::Options options;
                    options.min_filter = filament::backend::SamplerMinFilter::NEAREST_MIPMAP_LINEAR;
                    options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
                    options.wrap_mode = filament::backend::SamplerWrapMode::REPEAT;

                    auto texture_ptr_ = GetView().GetTextureFactory().CreateTexture(*texture, options);

                    material_ptr_->SetParameter("Texture", std::move(texture_ptr_));
                    material_ptr_->SetParameter("RainbowRingsThickness", rainbowEffectData_.rainbowRingsThickness);
                    material_ptr_->SetParameter("RainbowRingsSmoothness", rainbowEffectData_.rainbowRingsSmoothness);
                    material_ptr_->SetParameter("RainbowRadiusSmoothness",
                                                rainbowEffectData_.rainbowRadiusSmoothness);
                    material_ptr_->SetParameter("RainbowStartRenderingAngle", rainbowEffectData_.rainbowStartRenderingAngle);
                    material_ptr_->SetParameter("RainbowEndRenderingAngle", rainbowEffectData_.rainbowEndRenderingAngle);
                    material_ptr_->SetParameter("FadeStartAngle", rainbowEffectData_.fadeStartAngle);
                    material_ptr_->SetParameter("FadeEndAngle", rainbowEffectData_.fadeEndAngle);
                    material_ptr_->SetParameter("ColorIntensity", rainbowEffectData_.colorIntensity);
                    material_ptr_->SetParameter("Opacity", rainbowEffectData_.opacity);

                    renderer_model->SetMaterial(std::move(material_ptr_));
                }).KeptBy(this);
    }

    void RainbowEffectComponent::DestroyRainbowEffect() {
        GetView().DestroyNode(node);
        InitializeRainbowEffect();
    }


#if IMP_RUNTIME(DEV)
    // Customize Editor UI for this component
    void RainbowEffectComponent::DrawEditorUi() {
        if (ImGui::Button("Settings Update")) {
            DestroyRainbowEffect();
        }
    }
#endif // IMP_RUNTIME(DEV)
}
