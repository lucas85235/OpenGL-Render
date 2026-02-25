#include "imp.h"
#include "core/common/log.h"
#include "native/components/falling_leaves/falling_leaves_component.h"
#include "native/components/falling_leaves/falling_leaves_assets.h"

using namespace imp;
namespace ix::samsung::homecomponents
{
    void FallingLeavesComponent::Setup() {
        InitializeFallingLeaves();
    }

    void FallingLeavesComponent::InitializeFallingLeaves() {
        particle_instance_node_ = GetView().CreateNode();
        particle_instance_node_->SetName("Particle Node");
        particle_instance_node_->SetParentKeepWorldTransform(GetNode());

        fallingLeavesData_.first_color_01 = state_.first_color_01.value_or(fallingLeavesData_.first_color_01);
        fallingLeavesData_.second_color_01 = state_.second_color_01.value_or(fallingLeavesData_.second_color_01);
        fallingLeavesData_.first_color_02 = state_.first_color_02.value_or(fallingLeavesData_.first_color_02);
        fallingLeavesData_.second_color_02 = state_.second_color_02.value_or(fallingLeavesData_.second_color_02);
        fallingLeavesData_.first_color_03 = state_.first_color_03.value_or(fallingLeavesData_.first_color_03);
        fallingLeavesData_.second_color_03 = state_.second_color_03.value_or(fallingLeavesData_.second_color_03);
        fallingLeavesData_.size = state_.size.value_or(fallingLeavesData_.size);
        fallingLeavesData_.velocity = state_.velocity.value_or(fallingLeavesData_.velocity);
        for (int i = 0;i < state_.falling_leaves.size(); i++) {
            fallingLeavesData_.amount = state_.falling_leaves[i].amount.value_or(fallingLeavesData_.amount);
            fallingLeavesData_.position = state_.falling_leaves[i].position.value_or(fallingLeavesData_.position);
            fallingLeavesData_.rotation = state_.falling_leaves[i].rotation.value_or(fallingLeavesData_.rotation);
            fallingLeavesData_.length = state_.falling_leaves[i].length.value_or(fallingLeavesData_.length);
            InstanceFallingLeaves(fallingLeavesData_);
        };
    }

    void FallingLeavesComponent::InstanceFallingLeaves(FallingLeavesData fld_) {
        auto falling_leaves_mat_future = GetView().GetAssetManager().LoadMaterial(assets::kFallingLeavesCmat);
        auto falling_leaves_texture_future1 = GetView().GetAssetManager().LoadImage(assets::kTXLeaf1Part1V2Png);
        auto falling_leaves_texture_future2 = GetView().GetAssetManager().LoadImage(assets::kTXLeaf1Part2V2Png);
        auto falling_leaves_texture_future3 = GetView().GetAssetManager().LoadImage(assets::kTXLeaf2Part1V2Png);
        auto falling_leaves_texture_future4 = GetView().GetAssetManager().LoadImage(assets::kTXLeaf2Part2V2Png);
        auto falling_leaves_texture_future5 = GetView().GetAssetManager().LoadImage(assets::kTXLeaf3Part1V2Png);
        auto falling_leaves_texture_future6 = GetView().GetAssetManager().LoadImage(assets::kTXLeaf3Part2V2Png);
        auto falling_leaves_texture_future7 = GetView().GetAssetManager().LoadImage(assets::kTXLeafAltBaWPart1Png);
        auto falling_leaves_texture_future8 = GetView().GetAssetManager().LoadImage(assets::kTXLeafAltBaWPart2Png);

        falling_leaves_mat_future.Merge(falling_leaves_texture_future1, falling_leaves_texture_future2, falling_leaves_texture_future3,
                                        falling_leaves_texture_future4, falling_leaves_texture_future5, falling_leaves_texture_future6,
                                        falling_leaves_texture_future7, falling_leaves_texture_future8)
                .Then([=](std::tuple<AssetPtr<MaterialAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>,
                        AssetPtr<ImageAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>, AssetPtr<ImageAsset>> result){
                    auto [material, texture1, texture2, texture3, texture4, texture5, texture6, texture7, texture8] = result;

                    particle_instance_child_node_ = GetView().CreateNode();
                    particle_instance_child_node_->SetParentKeepWorldTransform(particle_instance_node_);

                    auto params = ParticleInstancesParams();

                    MeshQuad quad;
                    params.mesh = &quad;

                    params.amount = fld_.amount;
                    params.position = fld_.position;
                    params.rotation = fld_.rotation;
                    params.velocity = fld_.velocity;

                    imp::TextureFactory::Options options;
                    options.min_filter = filament::backend::SamplerMinFilter::LINEAR_MIPMAP_LINEAR;
                    options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
                    options.wrap_mode = filament::backend::SamplerWrapMode::REPEAT;

                    auto tx1 = GetView().GetTextureFactory().CreateTexture(*texture1, options);
                    auto tx2 = GetView().GetTextureFactory().CreateTexture(*texture2, options);
                    auto tx3 = GetView().GetTextureFactory().CreateTexture(*texture3, options);
                    auto tx4 = GetView().GetTextureFactory().CreateTexture(*texture4, options);
                    auto tx5 = GetView().GetTextureFactory().CreateTexture(*texture5, options);
                    auto tx6 = GetView().GetTextureFactory().CreateTexture(*texture6, options);
                    auto tx7 = GetView().GetTextureFactory().CreateTexture(*texture7, options);
                    auto tx8 = GetView().GetTextureFactory().CreateTexture(*texture8, options);
                    auto mat = GetView().GetMaterialFactory().CreateMaterial(material);

                    mat->SetParameter("Texture1", std::move(tx1));
                    mat->SetParameter("Texture2", std::move(tx2));
                    mat->SetParameter("Texture3", std::move(tx3));
                    mat->SetParameter("Texture4", std::move(tx4));
                    mat->SetParameter("Texture5", std::move(tx5));
                    mat->SetParameter("Texture6", std::move(tx6));
                    mat->SetParameter("Texture7", std::move(tx7));
                    mat->SetParameter("Texture8", std::move(tx8));
                    mat->SetParameter("FirstColor_01", fld_.first_color_01);
                    mat->SetParameter("SecondColor_01", fld_.second_color_01);
                    mat->SetParameter("FirstColor_02", fld_.first_color_02);
                    mat->SetParameter("SecondColor_02", fld_.second_color_02);
                    mat->SetParameter("FirstColor_03", fld_.first_color_03);
                    mat->SetParameter("SecondColor_03", fld_.second_color_03);
                    mat->SetParameter("particleSize", fld_.size);

                    params.material = std::move(mat);

                    auto domeShape = LinePosition(fld_.length);
                    params.positionFunction = domeShape.get();

                    particle_instance_child_node_->AddComponent<ParticleInstances>(params);

                }).KeptBy(this);
    }

    void FallingLeavesComponent::DestroyFallingLeaves() {
        GetView().DestroyNode(particle_instance_node_);
        InitializeFallingLeaves();
    }


#if IMP_RUNTIME(DEV)
        // Customize Editor UI for this component
        void FallingLeavesComponent::DrawEditorUi()
        {
            if (ImGui::Button("Settings Update")) {
                DestroyFallingLeaves();
            }
        }
    #endif // IMP_RUNTIME(DEV)
}
