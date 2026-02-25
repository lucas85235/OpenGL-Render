#include "native/components/buttons/icon_button/icon_button.h"

namespace ix::samsung::homecomponents
{
    void IconButton::Setup(float size) 
    {
        BaseButton::Setup
        ({
            .button_size = {size, size},
            .corner_radius = {50.0f},
            .color_rect = {1.0f, 1.0f, 1.0f}
        });
    }

    void IconButton::LoadIcon(const resources::ResourceDefinition& resource)
    {
        auto icon = GetView().CreateNode();
        auto path = resource.GetUrl();
        auto front_plate = GetFrontPlateNode();

        icon->SetParent(front_plate);
        icon->SetLocalPosition(float3(0));
        icon->SetLocalScale(float3(0.33f, 0.33f, 1.0f));
        icon->SetName("Icon");

        auto image_future = GetView().GetAssetManager().LoadImage(resource);
        auto material_future = GetView().GetAssetManager().LoadMaterial(assets::kIconMaterialCmat);
        image_future.Merge(material_future)
            .Then([this, icon](std::tuple<AssetPtr<imp::ImageAsset>,
                    AssetPtr<imp::MaterialAsset>> result)
            {
                auto [texture, material] = result;
                NodeHandle quad = CreateShapeNode(material, texture,
                                                  GetView().GetMeshFactory().CreateQuad({.size = {1, 1}, .center = {0, 0}, .z = 0.1}));
                quad->SetName("Image");
                quad->SetParent(icon);
            }).KeptBy(this);
    }

    // Helper function to create a node with a shape, material and a texture.
    NodeHandle IconButton::CreateShapeNode(AssetPtr<imp::MaterialAsset> material,
                               AssetPtr<imp::ImageAsset> texture,
                               MeshPtr shape_mesh)
    {
        auto shape_material = GetView().GetMaterialFactory().CreateMaterial(material);
        shape_material->SetParameter("BaseColor", GetView().GetTextureFactory().CreateTexture(*texture));

        NodeHandle shape = GetView().CreateNode();
        ComponentHandle<RenderComponent> shape_renderer =
                shape->AddComponent<RenderComponent>(
                        RenderComponent::FrustrumCullingMode::kDisabled);

        shape_renderer->SetMesh(std::move(shape_mesh));
        shape_renderer->SetMaterial(std::move(shape_material));
        return shape;
    }
}
