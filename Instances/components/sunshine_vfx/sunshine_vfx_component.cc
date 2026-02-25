#include <random>
#include <utility>

#include "imp.h"
#include "core/common/log.h"
#include "native/components/sunshine_vfx/sunshine_vfx_component.h"
#include "native/components/sunshine_vfx/sunshine_vfx_assets.h"

#if IMP_RUNTIME(DEV)
#include "dear_imgui/imgui.h"
#endif

using namespace imp;

namespace ix::samsung::homecomponents
{
    void SunshineVFXComponent::Setup() {
        InitializeVFX();
    }

    void SunshineVFXComponent::InitializeVFX()
    {
        // Early initializations
        noise_pattern_ = state_.noise_pattern.value_or(noise_pattern_);
        ray_pattern_ = state_.ray_pattern.value_or(ray_pattern_);
        ray_enabled_ =  state_.ray_enabled.value_or(ray_enabled_);
        ray_direction_ = state_.ray_direction.value_or(ray_direction_);
        bloom_enabled_ =  state_.bloom_enabled.value_or(bloom_enabled_);
        rooftop_enabled_ =  state_.rooftop_enabled.value_or(rooftop_enabled_);
        rooftop_color_ =  state_.rooftop_color.value_or(rooftop_color_);
        rooftop_plane_fade_speed_ = state_.rooftop_plane_fade_speed.value_or(rooftop_plane_fade_speed_);

        auto bloom_material_future = GetView().GetAssetManager().LoadMaterial(assets::kLightBloomWithNoiseMaterialCmat);
        auto ray_material_future = GetView().GetAssetManager().LoadMaterial(assets::kLightRayPlaneMaterialCmat);
        auto ray_gltf_future = GetView().GetAssetManager().LoadGltfAsset(assets::kDMSingleRayGlb);
        auto noise_pattern_future = GetView().GetAssetManager().LoadImage(noise_pattern_);
        auto ray_pattern_future = GetView().GetAssetManager().LoadImage(ray_pattern_);
        auto ray_pattern_50_future = GetView().GetAssetManager().LoadImage(ray_pattern_50_);
        auto ray_large_pattern_future = GetView().GetAssetManager().LoadImage(ray_large_pattern_);
        auto ray_large_gltf_future = GetView().GetAssetManager().LoadGltfAsset(assets::kSMGodlightGlb);
        auto rooftop_pattern_future = GetView().GetAssetManager().LoadImage(rooftop_pattern_);

        bloom_material_future.Merge(ray_material_future, ray_gltf_future,
                                    noise_pattern_future, ray_pattern_future, ray_large_gltf_future, ray_large_pattern_future, ray_pattern_50_future, rooftop_pattern_future)
            .Then([this](std::tuple<AssetPtr<imp::MaterialAsset>, AssetPtr<imp::MaterialAsset>,
                         AssetPtr<imp::GltfAsset>, AssetPtr<imp::ImageAsset>, AssetPtr<imp::ImageAsset>,  AssetPtr<imp::GltfAsset>, AssetPtr<imp::ImageAsset>, AssetPtr<imp::ImageAsset>, AssetPtr<imp::ImageAsset>> result){
                auto [bloom_material, ray_material, ray_gltf, noise_pattern_texture, ray_pattern_texture, ray_large_gltf, ray_large_pattern_texture, ray_50_pattern_texture, rooftop_pattern_texture] = result;

                NodeHandle owner = GetNode();
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<float> dis01(0.0f, 1.0f);
                TextureFactory::Options texture_options = {.wrap_mode = TextureFactory::WrapMode::REPEAT};

                // Direction targets
                ray_direction_target_ = GetView().CreateNode();
                ray_direction_target_->SetName("Ray Direction Target");
                ray_direction_target_->SetParent(owner);
                ray_direction_target_->SetLocalPosition(ray_direction_);

                // Bloom
                MaterialPtr bloom_material_ptr = GetView().GetMaterialFactory().CreateMaterial(bloom_material);
                bloom_material_ptr->SetParameter(TEXTURE, GetView().GetTextureFactory().CreateTexture(*ray_pattern_texture));
                bloom_material_ptr->SetParameter(NOISE_TEXTURE, GetView().GetTextureFactory().CreateTexture(*noise_pattern_texture, texture_options));
                bloom_material_ptr_ = bloom_material_ptr.get();
                bloom_node_ = CreateBloomNode(std::move(bloom_material_ptr));

                // Rooftop plane
                rooftop_node_ = GetView().CreateNode();
                rooftop_node_->SetName("RooftopPlane");
                rooftop_component_ = rooftop_node_->AddComponent<LightRay>("RooftopPlane", rooftop_pattern_texture,
                                                                           ray_material,
                                                                           rooftop_plane_alpha_range_,
                                                                           rooftop_plane_fade_speed_, rooftop_color_);
                rooftop_component_->CreateProceduralQuad({1.0, 1.0}, {0, 0});
                rooftop_node_->SetWorldPosition(rooftop_plane_position_);
                rooftop_node_->SetWorldRotation(QuatFromEuler(rooftop_plane_rotation_));
                rooftop_node_->SetWorldScale(rooftop_plane_scale_);
                rooftop_node_->SetParentKeepWorldTransform(GetNode());

                // Light rays
                NodeHandle ray_large = CreateSmallRay(
                        ray_large_gltf,
                        {-0.306, 1.136, -0.648},
                        { 0, 270, 359 },
                        { 0.4, 3.0, 1 },
                        ray_large_alpha_range_,
                        ray_large_pattern_texture,
                        ray_material,
                        "RayLarge");
                ray_large_material_ptr_ = ray_large->GetComponent<GltfRenderer>()->GetMaterialOverrideByIndex(0);

                NodeHandle ray_modelA = CreateSmallRay(
                        ray_gltf,
                        { -0.297, -1.707, -1.105 },
                        { 0, 90 ,12.5 },
                        { 0.8, 1.5, 0.155 },
                        ray_small_alpha_range_,
                        ray_50_pattern_texture,
                        ray_material,
                        "RayModelA");

                NodeHandle ray_modelB = CreateSmallRay(
                        ray_gltf,
                        {-0.001, -1.457, -0.096},
                        {0.0, 90.0, 5.5},
                        {0.350, 1.2, 0.155},
                        ray_small_alpha_range_,
                        ray_50_pattern_texture,
                        ray_material,
                        "RayModelB");

                // NodeHandle ray_modelC = CreateSmallRay(
                //         ray_gltf,
                //         {-0.035, 4.639, -0.291},
                //         {0.0, 90.0, 16.0},
                //         {0.150, 2.0, 0.155},
                //         ray_small_alpha_range_,
                //         ray_50_pattern_texture,
                //         ray_material,
                //         "RayModelC");

                OnIsfStateChanged();
                PrintMaterialParameters();
            }).KeptBy(this);
    }

    void SunshineVFXComponent::OnIsfStateChanged()
    {
        // Set default values for state variables without a value
        if(!state_.ray_direction) state_.ray_direction.emplace(ray_direction_);
        if(!state_.ray_enabled) state_.ray_enabled.emplace(ray_enabled_);
        if(!state_.noise_color) state_.noise_color.emplace(noise_color_);
        if(!state_.noise_direction) state_.noise_direction.emplace(noise_direction_);
        if(!state_.noise_speed) state_.noise_speed.emplace(noise_speed_);
        if(!state_.noise_pattern_tiling) state_.noise_pattern_tiling.emplace(noise_pattern_tiling_);
        if(!state_.ray_color) state_.ray_color.emplace(ray_color_);
        if(!state_.noise_pattern) state_.noise_pattern.emplace(noise_pattern_);
        if(!state_.ray_pattern) state_.ray_pattern.emplace(ray_pattern_);
        if(!state_.ray_fade_speed) state_.ray_fade_speed.emplace(ray_fade_speed_);
        if(!state_.ray_large_alpha_range) state_.ray_large_alpha_range.emplace(ray_large_alpha_range_);
        if(!state_.ray_small_alpha_range) state_.ray_small_alpha_range.emplace(ray_small_alpha_range_);
        if(!state_.bloom_enabled) state_.bloom_enabled.emplace(bloom_enabled_);
        if(!state_.bloom_color) state_.bloom_color.emplace(bloom_color_);
        if(!state_.bloom_brightness) state_.bloom_brightness.emplace(bloom_brightness_);
        if(!state_.bloom_fade_speed) state_.bloom_fade_speed.emplace(bloom_fade_speed_);
        if(!state_.bloom_alpha_range) state_.bloom_alpha_range.emplace(bloom_alpha_range_);
        if(!state_.rooftop_enabled) state_.rooftop_enabled.emplace(rooftop_enabled_);
        if(!state_.rooftop_color) state_.rooftop_color.emplace(rooftop_color_);
        if(!state_.rooftop_plane_fade_speed) state_.rooftop_plane_fade_speed.emplace(rooftop_plane_fade_speed_);

        UpdateLocalParameters();
        UpdateMaterialParameters();
    }

    void SunshineVFXComponent::UpdateLocalParameters()
    {
        // Set local variables from the current state
        ray_enabled_ = state_.ray_enabled.value();
        noise_color_ = state_.noise_color.value();
        noise_direction_ = state_.noise_direction.value();
        noise_speed_ = state_.noise_speed.value();
        ray_color_ = state_.ray_color.value();
        ray_large_alpha_range_ = state_.ray_large_alpha_range.value();
        ray_small_alpha_range_ = state_.ray_small_alpha_range.value();
        noise_pattern_tiling_ = state_.noise_pattern_tiling.value();
        ray_fade_speed_ = state_.ray_fade_speed.value();
        ray_direction_ = state_.ray_direction.value();
        bloom_enabled_ = state_.bloom_enabled.value();
        bloom_color_ = state_.bloom_color.value();
        bloom_brightness_ = state_.bloom_brightness.value();
        bloom_fade_speed_ = state_.bloom_fade_speed.value();
        bloom_alpha_range_ = state_.bloom_alpha_range.value();
        rooftop_enabled_ = state_.rooftop_enabled.value();
        rooftop_color_ = state_.rooftop_color.value();
        rooftop_plane_fade_speed_ = state_.rooftop_plane_fade_speed.value();
    }

    void SunshineVFXComponent::UpdateMaterialParameters()
    {
        ray_direction_offset_ = ray_direction_target_->GetWorldPosition() - GetNode()->GetWorldPosition();
        state_.ray_direction = ray_direction_target_->GetLocalPosition();

        for(NodeHandle node : ray_nodes_)
        {
            node->SetEnabled(ray_enabled_);
        }

        if(ray_enabled_)
        {
            for (Material *ptr: ray_material_ptrs_)
            {
                ptr->SetParameter(DIRECTION, ray_direction_offset_);
                ptr->SetParameter(COLOR, filament::RgbaType::sRGB, ray_color_);
                ptr->SetParameter(FADE_SPEED, ray_fade_speed_);
                ptr->SetParameter(ALPHA_RANGE, ray_small_alpha_range_);
            }
            ray_large_material_ptr_->SetParameter(ALPHA_RANGE, ray_large_alpha_range_);
        }

        // Bloom
        bloom_node_->SetEnabled(bloom_enabled_);
        if(bloom_enabled_)
        {
            bloom_material_ptr_->SetParameter(ALPHA_RANGE, bloom_alpha_range_);
            bloom_material_ptr_->SetParameter(BRIGHTNESS, bloom_brightness_);
            bloom_material_ptr_->SetParameter(COLOR, bloom_color_);
            bloom_material_ptr_->SetParameter(FADE_SPEED, bloom_fade_speed_);
            bloom_material_ptr_->SetParameter(NOISE_DIRECTION, noise_direction_);
            bloom_material_ptr_->SetParameter(NOISE_COLOR, filament::RgbaType::sRGB, noise_color_);
            bloom_material_ptr_->SetParameter(NOISE_SPEED, noise_speed_);
            bloom_material_ptr_->SetParameter(NOISE_TILING, noise_pattern_tiling_);
        }

        // Rooftop
        rooftop_node_->SetEnabled(rooftop_enabled_);
        rooftop_component_->SetColor(rooftop_color_);
        rooftop_component_->SetPlaneFadeSpeed(rooftop_plane_fade_speed_);
    }

    void SunshineVFXComponent::PrintMaterialParameters(){
        IMP_LOG(INFO) << "Current material parameter values for " << GetNode()->GetName() << ": \n"
            << "noiseColor:" << noise_color_ << "\n"
            << "noiseDirection:" << noise_direction_ << "\n"
            << "noisePatternTiling:" << noise_pattern_tiling_ << "\n"
            << "noiseSpeed:" << noise_speed_ << "\n"
            << "rayColor:" << ray_color_ << "\n"
            << "noisePattern:" << noise_pattern_ << "\n";
    }

    // Updates shineDirection, so it points to the shine_direction_target_ position
    // Called by an Editor button but can be called directly if needed
    void SunshineVFXComponent::UpdateShineDirection()
    {
        UpdateLocalParameters();
        UpdateMaterialParameters();
    }

    void SunshineVFXComponent::DisableGltfColliders(NodeHandle node){
        if(node->GetComponent<GltfCollider>().IsValid()){
            node->GetComponent<GltfCollider>()->SetEnabled(false);
        }
        for(auto childNode : node->GetChildren()){
            DisableGltfColliders(childNode);
        }
    }

    Box SunshineVFXComponent::Union(std::optional<Box> a, const Box &b) {
        if (!a.has_value()) return b;
        return a->unionSelf(b);
    }

    std::optional<Box> SunshineVFXComponent::GetBounds(NodeHandle node, std::optional<Box> result) {
        ComponentHandle<GltfMesh> gltf_mesh = node->GetComponent<GltfMesh>();
        if (gltf_mesh) {
            result = Union(result, gltf_mesh->GetLocalBounds());
        }

        for (NodeHandle child: node->GetChildren()) {
            result = GetBounds(child, result);
        }

        return result;
    }

    NodeHandle SunshineVFXComponent::CreateSmallRay(
            const AssetPtr<GltfAsset>& gltf,
            float3 position,
            float3 rotation,
            float3 scale,
            float2 alpha_range,
            const AssetPtr<ImageAsset>& texture,
            const AssetPtr<MaterialAsset>& material,
            const std::string& name)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis01(0.0f, 1.0f);
        float3 color(ray_color_.x, ray_color_.y, ray_color_.z);

        NodeHandle ray_model = GetView().CreateNode();

        ray_model->SetName(name);
        auto light_ray_component_ =
                ray_model->AddComponent<LightRay>(name, texture, material, alpha_range, ray_fade_speed_, color);
        light_ray_component_->CreateFromGltfAsset(gltf);
        ray_model->SetParent(GetNode());
        ray_model->SetLocalPosition(position);
        ray_model->SetLocalRotation(QuatFromEuler(rotation));
        ray_model->SetLocalScale(scale);

        auto ray_material = ray_model->GetComponent<GltfRenderer>()->GetMaterialOverrideByIndex(0);
        ray_material->SetParameter(FADE_SPEED, ray_fade_speed_);
        ray_material->SetParameter(FADE_OFFSET, dis01(gen) > 0.5f ? 0.0f : PI);
        ray_material_ptrs_.push_back(ray_material);
        ray_nodes_.push_back(ray_model);

        return ray_model;
    }

    NodeHandle SunshineVFXComponent::CreateBloomNode(MaterialPtr material)
    {
        MeshPtr shape_mesh = GetView().GetMeshFactory().CreateQuad({.size = {12.0, 12.0}, .center = {0.5, 0.5}, .flip_uv = true});
        NodeHandle shape = GetView().CreateNode();
        ComponentHandle<RenderComponent> shape_renderer =
                shape->AddComponent<RenderComponent>(
                        RenderComponent::FrustrumCullingMode::kDisabled);

        float3 bloom_model_position = bloom_position_;
        quatf bloom_model_rotation = QuatFromEuler (bloom_rotation_);
        float3 bloom_model_scale = bloom_scale_;
        shape->SetLocalPosition(bloom_model_position);
        shape->SetLocalRotation(bloom_model_rotation);
        shape->SetLocalScale(bloom_model_scale);
        shape->SetName("Bloom");

        shape_renderer->SetMesh(std::move(shape_mesh));
        shape_renderer->SetMaterial(std::move(material));
        return shape;
    }

#if IMP_RUNTIME(DEV)
    void SunshineVFXComponent::DrawEditorUi()
    {
        // if (ImGui::Button("Update Ray Direction"))
        // {
        //     UpdateShineDirection();
        // }
        if (ImGui::Button("Toggle Ray effect"))
        {
            state_.ray_enabled = !state_.ray_enabled.value();
            OnIsfStateChanged();
        }
        // if (ImGui::Button("Toggle Bloom effect"))
        // {
        //     state_.bloom_enabled = !state_.bloom_enabled.value();
        //     OnIsfStateChanged();
        // }

        if (ImGui::Button("Toggle Rooftop effect"))
        {
            state_.rooftop_enabled = !state_.rooftop_enabled.value();
            OnIsfStateChanged();
        }
    }
#endif // IMP_RUNTIME(DEV)
}
