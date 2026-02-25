#include "core/common/log.h"
#include "imp.h"
#include "native/components/fireworks/fireworks.h"
#include "native/components/fireworks/fireworks_assets.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void Fireworks::Setup()
    {
        state_.color = state_.color.value_or(float4(1.0));

        Initialize();
    }

    void Fireworks::Initialize()
    {
        auto sprite_material_future = GetView().GetAssetManager().LoadMaterial(assets::kFireworksMaterialCmat);
        auto sprite_future = GetView().GetAssetManager().LoadImage(assets::kTXSquarePng);

        sprite_material_future.Merge(sprite_future)
            .Then([this](std::tuple<AssetPtr<MaterialAsset>, AssetPtr<ImageAsset>> result) {
                auto [material_asset, texture_asset] = std::move(result);

                fireworks_node_ = GetView().CreateNode();
                CreateQuadSettings settings;
                settings.size = {2, 2};
                auto sprite_mesh = GetView().GetMeshFactory().CreateQuad(settings);
                fireworks_node_->SetName("FireworksSprite");
                fireworks_node_->SetParent(GetNode());
                auto renderer_model = fireworks_node_->AddComponent<RenderComponent>(RenderComponent::FrustrumCullingMode::kDisabled);
                renderer_model->SetMesh(std::move(sprite_mesh));
                renderer_model->SetPriority(1);

                auto material = GetView().GetMaterialFactory().CreateMaterial(material_asset);
                material->SetParameter("BaseTexture", GetView().GetTextureFactory().CreateTexture(*texture_asset));
                material->SetParameter("Color", state_.color.value());
                renderer_model->SetMaterial(std::move(material));
            }).KeptBy(this);
    }

    void Fireworks::Update(const FrameTime& frame_time)
    {
    }

    void Fireworks::Play()
    {
        output::Error("Fireworks: playing...");
    }

#if IMP_RUNTIME(DEV)
    // Customize Editor UI for this component
    void Fireworks::DrawEditorUi()
    {
       if (ImGui::Button("Reload effect"))
       {
           GetView().DestroyNode(fireworks_node_);
           Initialize();
       }
    }
#endif
}
