#include "native/components/base_button/base_button.h"
#include "native/components/base_button/base_button_assets.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void BaseButton::Setup() {}

    void BaseButton::Setup(ButtonProperties properties)
    {
        properties_ = properties;

        auto node = GetNode();
        node->SetName("BaseButton");

        frontplate_node_ = GetView().CreateNode();
        frontplate_node_->SetName("Frontplate");
        frontplate_node_->SetParent(node);
        frontplate_node_->SetLocalPosition({0.0f, 0.0f, 0.02});

        backplate_node_ = GetView().CreateNode();
        backplate_node_->SetName("Backplate");
        backplate_node_->SetParent(node);

        Future<NodeHandle> button_future = GetView().GetAssetManager().LoadModel(assets::kSMRoundedCornerGlb);
        Future<NodeHandle> highlight_future = GetView().GetAssetManager().LoadModel(assets::kSMRoundedCornerBorderGlb);
        
        auto material_future = GetView().GetAssetManager().LoadMaterial(assets::k3dButtonCmat);
        material_future.Merge(button_future, highlight_future)
            .Then([this](std::tuple<AssetPtr<MaterialAsset>, NodeHandle, NodeHandle> result)
            {
                auto [material, model_button, model_highlight] = result;
                // Setup backplate material
                model_button->SetName("SM_RoundedCorner");
                model_button->SetParent(backplate_node_);

                auto gltf_mesh = model_button->GetChildren()[0]->GetChildren()[0];
                gltf_mesh->GetComponent<GltfCollider>()->SetMask(CollisionMask::kStatic);

                auto gltf_render = backplate_node_->FindByName("SM_RoundedCorner")->GetComponent<GltfRenderer>();
                Box world_bounds = gltf_render->GetLocalBounds();
                backplate_node_->AddComponent<BoxCollider>(world_bounds);

                auto material_ptr = GetView().GetMaterialFactory().CreateMaterial(material);
                material_ptr->SetParameter("BaseColor", filament::RgbType::LINEAR, float3(1.0f, 1.0f, 1.0f));
                material_ptr->SetParameter("Alpha", 1.0f);

                gltf_render->SetMaterialOverrideByIndex(std::move(material_ptr), 0);
                button_material_ = std::move(gltf_render->GetMaterialOverrideByIndex(0));

                // Setup highlight material
                model_highlight->SetName("SM_RoundedCornerBorder");
                model_highlight->SetParent(backplate_node_);
                highlight_node_ = model_highlight;
                highlight_node_->SetLocalPosition(initial_position_);

                auto gltf_highlight_mesh = model_highlight->GetChildren()[0]->GetChildren()[0];
                gltf_highlight_mesh->GetComponent<GltfCollider>()->SetMask(CollisionMask::kStatic);

                auto gltf_highlight_render = backplate_node_->FindByName("SM_RoundedCornerBorder")->GetComponent<GltfRenderer>();

                auto material_highlight_ptr = GetView().GetMaterialFactory().CreateMaterial(material);
                material_highlight_ptr->SetParameter("BaseColor", filament::RgbType::LINEAR, float3(0.0f, 0.0f, 1.0f));
                material_highlight_ptr->SetParameter("Alpha", 0.0f);

                gltf_highlight_render->SetMaterialOverrideByIndex(std::move(material_highlight_ptr), 0);
                highlight_material_ = std::move(gltf_highlight_render->GetMaterialOverrideByIndex(0));

            }).KeptBy(this);


        interaction_ = backplate_node_->AddComponent<Interactable>(0,1,0,0);
        interaction_->Connect([=](const InteractionStateChanged &event) mutable
        {
            auto state = interaction_->GetInteractionState();

            if (state == State::NORMAL)
            {
                BaseButton::ButtonNormal();
            }
            else if (state == State::HOVER_ENTER)
            {
                BaseButton::ButtonHovered();
            }
            else if (state == State::CLICKED_DOWN)
            {
                BaseButton::ButtonClicked();
            }
            else if (state == State::CLICKED_UP)
            {
            }
        });

        node->SetLocalScale({properties_.button_size, 0.08f});
    }

    void BaseButton::Update(const FrameTime &frame_time)
    {

        if(!is_in_lerp_)
        {
            return;
        }

        float3 position = lerp(initial_position_, final_position_, elapsed_ / duration_);
        highlight_node_->SetLocalPosition(position);
        elapsed_ += frame_time.GetDeltaSeconds();

        if(elapsed_ >= duration_ / 2)
        {
            button_material_->SetParameter("BaseColor", filament::RgbType::LINEAR, float3(0.0f, 0.0f, 1.0f));
        }

        if(elapsed_ >= duration_)
        {
            button_material_->SetParameter("BaseColor", filament::RgbType::LINEAR, float3(1.0f, 1.0f, 1.0f));
            highlight_node_->SetLocalPosition(initial_position_);
            is_in_lerp_ = false;
            elapsed_ = 0.0f;
        }

        ChildUpdate();
    }

    void BaseButton::ButtonNormal()
    {
        if(is_in_lerp_)
        {
            return;
        }

        button_material_->SetParameter("BaseColor", filament::RgbType::LINEAR, float3(1.0f, 1.0f, 1.0f));
        highlight_material_->SetParameter("Alpha", 0.0f);
        highlight_node_->SetLocalPosition(initial_position_);
    }

    void BaseButton::ButtonHovered()
    {
        if(is_in_lerp_)
        {
            return;
        }

        // Does not work on Android
        button_material_->SetParameter("BaseColor", filament::RgbType::LINEAR, float3(1.0f, 1.0f, 1.0f));
        highlight_material_->SetParameter("Alpha", 1.0f);
    }

    void BaseButton::ButtonClicked()
    {
        if(is_in_lerp_)
        {
            return;
        }

        is_in_lerp_ = true;
        highlight_material_->SetParameter("Alpha", 1.0f);
    }

    NodeHandle BaseButton::GetFrontPlateNode()
    {
        return frontplate_node_;
    }

    void BaseButton::ChildUpdate()
    {

    }
}
