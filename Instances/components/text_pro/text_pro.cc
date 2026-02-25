#include "native/components/text_pro/text_pro.h"
#include <memory>
#include <optional>
#include <utility>
#include <filesystem>
#include <tuple>
#include "imp.h"
#include "text_pro.h"

namespace ix::samsung::homecomponents {
    void TextPro::Setup() {
        //GetNode()->SetWorldPosition(float3(0.0, 0.0, -0.5));
    }

    void TextPro::CreatePanel(const resources::ResourceDefinition &image, const resources::ResourceDefinition &mat, float size) {
        auto image_future = GetView().GetAssetManager().LoadImage(image);
        auto mat_future = GetView().GetAssetManager().LoadMaterial(mat);

        image_future.Merge(mat_future)
                .Then([=](
                std::tuple<AssetPtr<imp::ImageAsset>, AssetPtr<imp::MaterialAsset> > result) {
                    auto [texture, material] = std::move(result);
                    quad_node_ = CreateShapeNode(material, texture,
                          GetView().GetMeshFactory().CreateQuad(
                              {
                                  .size = size,
                                  .center = float2(0.0f,0.0f),
                                  .z = 0.0
                              }));
                    }).KeptBy(this);
    }

    NodeHandle TextPro::CreateShapeNode(const AssetPtr<imp::MaterialAsset> &material,
                                        const AssetPtr<imp::ImageAsset> &texture,
                                        MeshPtr shape_mesh) {
        NodeHandle shape = GetView().CreateNode();
        text_material_ = GetView().GetMaterialFactory().CreateMaterial(material);

        text_material_->SetParameter("BaseColor", GetView().GetTextureFactory().CreateTexture(*texture));
        text_material_->SetParameter("TextValueMapping", std::move(mapping_texture_));
        text_material_->SetParameter("AtlasSize", char_length_);
        text_material_->SetParameter("StringLength", static_cast<int>(chars_.size()));
        text_material_->SetParameter("FontSize", default_font_size_);
        text_material_->SetParameter("MinSoftEdge", min_soft_edge_);
        text_material_->SetParameter("MaxSoftEdge", max_soft_edge_);

        ComponentHandle<RenderComponent> shape_renderer =
                shape->AddComponent<RenderComponent>(
                    RenderComponent::FrustrumCullingMode::kDisabled);

        shape_renderer->SetMesh(std::move(shape_mesh));
        shape_renderer->SetMaterial(text_material_.get());
        shape_renderer->SetPriority(7);
        shape->SetParent(GetNode());
        shape->SetLocalPosition(float3(0.0));
        return shape;
    }

    void TextPro::SetText(const string &text, const float size) {
        chars_ = text;
        mapping_texture_ = CreateMappingTexture();

        // fonts
        CreatePanel(assets::kNotoPng, assets::kTextMaterialCmat, size);
    }

    void TextPro::SetTextPostInitialized(string text) {
        chars_ = std::move(text);
        mapping_texture_ = CreateMappingTexture();

        if (!text_material_) return;
        text_material_->SetParameter("TextValueMapping", mapping_texture_.get());
        text_material_->SetParameter("AtlasSize", char_length_);
        text_material_->SetParameter("StringLength", static_cast<int>(chars_.size()));
        quad_node_->SetWorldScale(float3(state_.text_field_size.value_or(1.0)));
    }

    void TextPro::SetFontSize(float size) {
        font_size_ = size;
    }

    imp::TexturePtr TextPro::CreateMappingTexture() {
        //get the max efficient texture size based on the amount of characters
        char_length_ = std::ceil(std::sqrt(chars_.length()));

        //initial size of the data atlas
        int width = char_length_;
        int height = char_length_;
        value_data.resize(width * height, ' ');

        int index = 0;

        for (int i = 0; i < chars_.size(); i++) {
            value_data[index] = chars_[i];
            index++;
        }

#pragma region TextTextureCreation
        //Creates a sample texture with the same size of the texture data
        imp::TexturePtr tex = GetView().GetTextureFactory().CreateTexture(static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                                                                          TextureFactory::Format::R8, filament::Texture::Usage::SAMPLEABLE, {
                                                                              .mag_filter = TextureFactory::MagFilter::NEAREST,
                                                                              .min_filter = TextureFactory::MinFilter::LINEAR
                                                                          });

        filament::backend::PixelDataType type = filament::Texture::Type::UBYTE;

        size_t size = (size_t) (width * height * sizeof(byte));
        filament::Texture::PixelBufferDescriptor buffer(value_data.data(), size, filament::Texture::Format::R, type);
        tex->GetTexture()->setImage(*GetView().GetSharedEngine(), 0, std::move(buffer));
#pragma endregion

        return tex;
    }

#if IMP_RUNTIME(DEV)
    // Customize Editor UI for this component
    void TextPro::DrawEditorUi() {

        if (ImGui::Button("Update Text")) {
            //material params
            text_material_->SetParameter("FontSize", state_.font_size.value_or(default_font_size_));
            text_material_->SetParameter("MaxSoftEdge", state_.max_soft_edge.value_or(max_soft_edge_));
            text_material_->SetParameter("MinSoftEdge", state_.min_soft_edge.value_or(min_soft_edge_));

            if (!string(state_.text_value).empty()) {
                SetTextPostInitialized(state_.text_value);
            } else {
                SetTextPostInitialized(" ");
            }
        }
    }
#endif
} // ix::samsung::homecomponents
