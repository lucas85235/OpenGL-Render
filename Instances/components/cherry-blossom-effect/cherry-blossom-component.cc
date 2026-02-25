#include "cherry-blossom-component.h"
#include "native/components/cherry-blossom-effect/cherry_blossom_demo_assets.h"
#include "imp.h"

using namespace imp;

namespace ix::samsung::homecomponents {
    void CherryBlossomComponent::InitializeParticles() {
        cherry_blossoms_data_.amount = state_.amount.value_or(cherry_blossoms_data_.amount);
        cherry_blossoms_data_.velocity = state_.velocity.value_or(cherry_blossoms_data_.velocity);
        cherry_blossoms_data_.radius = state_.radius.value_or(cherry_blossoms_data_.radius);
        cherry_blossoms_data_.max_animation_time = state_.maxAnimationTime.value_or(cherry_blossoms_data_.max_animation_time);
        cherry_blossoms_data_.emission_rate = state_.emissionRate.value_or(cherry_blossoms_data_.emission_rate);
        cherry_blossoms_data_.size = state_.size.value_or(cherry_blossoms_data_.size);
        cherry_blossoms_data_.first_color = state_.firstColor;
        cherry_blossoms_data_.second_color = state_.secondColor;
        cherry_blossoms_data_.color_blend_amount = state_.colorBlendAmount.value_or(cherry_blossoms_data_.color_blend_amount);
        InstanceParticles();
    }

    void CherryBlossomComponent::InstanceParticles() {
        auto image = GetView().GetAssetManager().LoadImage(cherry_blossom_demo_data::kPetalsPng);
        auto image2 = GetView().GetAssetManager().LoadImage(cherry_blossom_demo_data::kPetals2Png);
        auto image3 = GetView().GetAssetManager().LoadImage(cherry_blossom_demo_data::kPetals3Png);
        auto material = GetView().GetAssetManager().LoadMaterial(cherry_blossom_demo_data::kCherryBlossomCmat);
        material.Merge(image, image2, image3).Then(
            [=](std::tuple<imp::AssetPtr<imp::MaterialAsset>, imp::AssetPtr<imp::ImageAsset>, imp::AssetPtr<imp::ImageAsset>, imp::AssetPtr<imp::ImageAsset> > result) {
                auto [material, image, image2,image3] = result;

                auto params = ParticleInstancesParams();
                std::vector<AssetPtr<ImageAsset> > images;
                images.push_back(image);
                images.push_back(image2);
                images.push_back(image3);

                particle_instances_node_ = GetView().CreateNode();
                particle_instances_node_->SetName("particleInstancesNode");
                particle_instances_node_->SetParent(GetNode());
                TexturePtr petal_texture = GetView().GetTextureFactory().CreateTexture(*images.at(0));
                TexturePtr petal_texture2 = GetView().GetTextureFactory().CreateTexture(*images.at(1));
                TexturePtr petal_texture3 = GetView().GetTextureFactory().CreateTexture(*images.at(2));
                params.amount = cherry_blossoms_data_.amount;
                params.position = GetNode()->GetWorldPosition();
                params.size = cherry_blossoms_data_.size;
                params.velocity = cherry_blossoms_data_.velocity;
                params.material = GetView().GetMaterialFactory().CreateMaterial(material);

                params.material->SetParameter("BaseTexture", std::move(petal_texture));
                params.material->SetParameter("AnimationTexture1", std::move(petal_texture2));
                params.material->SetParameter("AnimationTexture2", std::move(petal_texture3));
                params.material->SetParameter("MaxAnimationTime", cherry_blossoms_data_.max_animation_time);
                params.material->SetParameter("EmissionRate", cherry_blossoms_data_.emission_rate);
                params.material->SetParameter("SecondColor", cherry_blossoms_data_.second_color);
                params.material->SetParameter("FirstColor", cherry_blossoms_data_.first_color);
                params.material->SetParameter("BlendColorAmount", cherry_blossoms_data_.color_blend_amount);

                MeshQuad mesh_quad;
                params.mesh = &mesh_quad;

                auto circle_shape = CirclePosition(cherry_blossoms_data_.radius);
                params.positionFunction = circle_shape.get();

                particle_instances_node_->AddComponent<ParticleInstances>(params);
            }).KeptBy(this);
    }

    absl::Status CherryBlossomComponent::Setup() {
        InitializeParticles();
        return absl::OkStatus();
    }

    void CherryBlossomComponent::Update(const FrameTime &frame_time) {
    }

    void CherryBlossomComponent::RestartStars() {
        GetView().DestroyNode(particle_instances_node_);
        InitializeParticles();
    }

#if IMP_RUNTIME(DEV)
    // Customize Editor UI for this component
    void CherryBlossomComponent::DrawEditorUi()
    {
        if (ImGui::Button("Settings Update")) {
            RestartStars();
        }
    }
#endif // IMP_RUNTIME(DEV)
} // namespace xr::component
