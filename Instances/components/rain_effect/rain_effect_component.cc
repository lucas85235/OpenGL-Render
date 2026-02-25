#include "imp.h"
#include "core/common/log.h"
#include "native/components/rain_effect/rain_effect_component.h"
#include "native/components/rain_effect/rain_effect_assets.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void RainEffectComponent::Setup() {
        InitializeRain();
    }

    void RainEffectComponent::InitializeRain() {
        particle_instance_node_ = GetView().CreateNode();
        particle_instance_node_->SetName("Particle Node");
        particle_instance_node_->SetParentKeepWorldTransform(GetNode());

        rainEffectData_.size = state_.size.value_or(rainEffectData_.size);
        rainEffectData_.velocity = state_.velocity.value_or(rainEffectData_.velocity);
        rainEffectData_.angle = state_.angle.value_or(rainEffectData_.angle);

        if (state_.dome.id.has_value()) {
            output::Info("DOME: ");
            rainEffectData_.id = state_.dome.id.value_or(rainEffectData_.id);
            rainEffectData_.amount = state_.dome.amount.value_or(rainEffectData_.amount);
            rainEffectData_.radius = state_.dome.radius.value_or(rainEffectData_.radius);
            InstanceRains(rainEffectData_);
        }

        if (state_.center.id.has_value()) {
            output::Info("CENTER: ");
            rainEffectData_.id = state_.center.id.value_or(rainEffectData_.id);
            rainEffectData_.amount = state_.center.amount.value_or(rainEffectData_.amount);
            rainEffectData_.radius = state_.center.radius.value_or(rainEffectData_.radius);
            InstanceRains(rainEffectData_);
        }
    }

    void RainEffectComponent::InstanceRains(RainEffectData red_) {
        auto load = GetView().GetAssetManager().LoadMaterial(assets::kRainMaterialCmat);

        load.Then([=](AssetPtr<MaterialAsset> material) {
            particle_instance_child_node_ = GetView().CreateNode();
            particle_instance_child_node_->SetParentKeepWorldTransform(particle_instance_node_);
            auto params = ParticleInstancesParams();

            MeshDoubleQuad doubleQuad = MeshDoubleQuad(1.0);
            params.mesh = &doubleQuad;

            params.amount = red_.amount;
            params.material = GetView().GetMaterialFactory().CreateMaterial(material);
            params.size = float2(red_.size, red_.size);
            params.radius = red_.radius;
            params.velocity = float2(red_.velocity, red_.velocity);
            params.angle = red_.angle;

            float ring_closed = 0.0;
            red_.id == 0 ? ring_closed = -0.00225 * red_.radius + 0.0725 : ring_closed = 0.0;
            red_.id == 0 ? params.center = false : params.center = true;

            auto domeShape = RingPosition(1.0, ring_closed);
            params.positionFunction = domeShape.get();

            particle_instance_child_node_->AddComponent<ParticleInstances>(params);
        }).KeptBy(this);
    }

    void RainEffectComponent::DestroyRain() {
        GetView().DestroyNode(particle_instance_node_);
        InitializeRain();
    }

#if IMP_RUNTIME(DEV)
    // Customize Editor UI for this component
    void RainEffectComponent::DrawEditorUi()
    {
        if (ImGui::Button("Settings Update")) {
            DestroyRain();
        }
    }
#endif // IMP_RUNTIME(DEV)
}
