#include "imp.h"
#include "core/common/log.h"
#include "native/components/twinkle_stars/twinkle_stars_component.h"
#include "native/components/twinkle_stars/twinkle_stars_assets.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void TwinkleStarsComponent::Setup() {
        InitializeStars();
    }

    void TwinkleStarsComponent::InitializeStars() {
        particle_instance_node_ = GetView().CreateNode();
        particle_instance_node_->SetName("Particle Node");
        particle_instance_node_->SetParentKeepWorldTransform(GetNode());

        twinkleStarsData_.velocity = state_.velocity.value_or(twinkleStarsData_.velocity);
        twinkleStarsData_.size = state_.size.value_or(twinkleStarsData_.size);

        if (state_.dome.id.has_value()) {
            twinkleStarsData_.id = state_.dome.id.value_or(twinkleStarsData_.id);
            twinkleStarsData_.amount = state_.dome.amount.value_or(twinkleStarsData_.amount);
            twinkleStarsData_.radius = state_.dome.radius.value_or(twinkleStarsData_.radius);
            twinkleStarsData_.position = state_.dome.position.value_or(twinkleStarsData_.position);
            twinkleStarsData_.rotation = state_.dome.rotation.value_or(twinkleStarsData_.rotation);
            twinkleStarsData_.radial = state_.dome.radial.value_or(twinkleStarsData_.radial);
            twinkleStarsData_.angular = state_.dome.angular.value_or(twinkleStarsData_.angular);
            twinkleStarsData_.color = state_.dome.color;
            InstanceStars(twinkleStarsData_);
        }

        for (int i=0;i<state_.range.size();i++) {
            if (state_.range[i].id.has_value()) {
                twinkleStarsData_.id = state_.range[i].id.value_or(twinkleStarsData_.id);
                twinkleStarsData_.amount = state_.range[i].amount.value_or(twinkleStarsData_.amount);
                twinkleStarsData_.radius = state_.range[i].radius.value_or(twinkleStarsData_.radius);
                twinkleStarsData_.position = state_.range[i].position.value_or(twinkleStarsData_.position);
                twinkleStarsData_.rotation = state_.range[i].rotation.value_or(twinkleStarsData_.rotation);
                twinkleStarsData_.radial = state_.range[i].radial.value_or(twinkleStarsData_.radial);
                twinkleStarsData_.angular = state_.range[i].angular.value_or(twinkleStarsData_.angular);
                twinkleStarsData_.color = state_.range[i].color;
                InstanceStars(twinkleStarsData_);
            }
        }

        if (state_.circle.id.has_value()) {
            twinkleStarsData_.id = state_.circle.id.value_or(twinkleStarsData_.id);
            twinkleStarsData_.amount = state_.circle.amount.value_or(twinkleStarsData_.amount);
            twinkleStarsData_.radius = state_.circle.radius.value_or(twinkleStarsData_.radius);
            twinkleStarsData_.position = state_.circle.position.value_or(twinkleStarsData_.position);
            twinkleStarsData_.rotation = state_.circle.rotation.value_or(twinkleStarsData_.rotation);
            twinkleStarsData_.color = state_.circle.color;
            InstanceStars(twinkleStarsData_);
        }
    }

    void TwinkleStarsComponent::InstanceStars(TwinkleStarsData tsd_) {
        auto material = GetView().GetAssetManager().LoadMaterial(assets::kTwinkleStarInstancedCmat);
        material.Then([=](imp::AssetPtr<imp::MaterialAsset> material) {
            particle_instance_child_node_ = GetView().CreateNode();
            particle_instance_child_node_->SetParentKeepWorldTransform(particle_instance_node_);
            auto params = ParticleInstancesParams();

            MeshCube iso_cube;
            params.mesh = &iso_cube;
            params.amount = tsd_.amount;
            params.material = GetView().GetMaterialFactory().CreateMaterial(material);
            params.velocity = float2(tsd_.velocity * 0.5, tsd_.velocity * 2.0);
            params.radius = tsd_.radius;
            params.size = float2(tsd_.size / 1.015, tsd_.size * 1.015);
            params.position = tsd_.position;
            params.color = tsd_.color;

            if (tsd_.id == 0 || tsd_.id == 1) {
                tsd_.id == 0 ? particle_instance_child_node_->SetName("Dome") : particle_instance_child_node_->SetName("Range");
                SpherePosition domeShape = SpherePosition(1.0f, tsd_.radial, tsd_.angular);
                params.positionFunction = domeShape.get();
                particle_instance_child_node_->AddComponent<ParticleInstances>(params);
            }

            if (tsd_.id == 2) {
                particle_instance_child_node_->SetName("Circle");
                CirclePositionStars circleShape = CirclePositionStars(1.0, 0.5);
                params.positionFunction = circleShape.get();
                particle_instance_child_node_->AddComponent<ParticleInstances>(params);
            }


        }).KeptBy(this);
    }

    void TwinkleStarsComponent::DestroyStars() {
        GetView().DestroyNode(particle_instance_node_);
        InitializeStars();
    }

#if IMP_RUNTIME(DEV)
        // Customize Editor UI for this component
        void TwinkleStarsComponent::DrawEditorUi()
        {
            if (ImGui::Button("Settings Update")) {
                DestroyStars();
            }
        }
    #endif // IMP_RUNTIME(DEV)
}
