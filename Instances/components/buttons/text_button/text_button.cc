#include "native/components/buttons/text_button/text_button.h"

namespace ix::samsung::homecomponents
{
    TextButton::TextButton() {
        kHeaderTextOptions_ = ScopedCanvas::TextOptions {
            .size_pixels = 150,
            .horizontal_alignment = ScopedCanvas::TextHorizontalAlignment::kCenter,
            .vertical_alignment = ScopedCanvas::TextVerticalAlignment::kCenter,
            .color = {1.0f, 5.0f, 0.0f, 1.0f},
            .stroke_width_pixels = 1,
            .stroke_color = {255.0f, 255.0f, 0.0f, 1.0f}
        };
    }

    void TextButton::Setup(float2 size, absl::string_view text)
    {
        BaseButton::Setup
        ({
            .button_size = size,
        });

        kTextHeader = text;

        canvas_source_ = CanvasSource::Create(GetView());
        auto front_plate = GetFrontPlateNode();

        NodeHandle text_ = GetView().CreateNode();
        text_->SetParent(front_plate);
        text_->SetLocalPosition({0.0f, 0.0f, 0.180f}); //float3(0)
        text_->SetLocalScale(float3(0.33f, 0.33f, 1.0f));
        text_->SetName("Text");

        GetView().GetAssetManager()
            .LoadMaterial(assets::kTextMaterialCmat)
            .Then([this, text_](absl::StatusOr<AssetPtr<MaterialAsset>> material) mutable
            {
                if (!material.ok()) {
                  output::Error("Unable to load material: %s",
                                material.status().ToString());
                }

                // Setup the quad and assign the material+texture to render to it.
                render_component_ = text_->AddComponent<RenderComponent>();
                render_component_->SetMesh(GetView().GetMeshFactory().CreateQuad());
                render_component_->SetMaterial(GetView().GetMaterialFactory().CreateMaterial(*material));

                UpdateText();
            })
            .KeptBy(this);
    }

    void TextButton::SetNewColor(float4 color)
    {
        kHeaderTextOptions_.color = color;
        UpdateText();
    }

    void TextButton::UpdateText()
    {
        if (canvas_source_ && render_component_)
        {
            float2 textPos = {512, 256};

            std::unique_ptr<ScopedCanvas> canvas = canvas_source_->StartDrawing({1024, 512});
            render_component_->GetMaterial()->SetParameter("BaseColor", canvas->GetTexture());

            canvas->DrawText(kTextHeader, textPos, kHeaderTextOptions_);
        }
    }
}