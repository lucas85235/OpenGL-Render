#include "imp.h"
#include "core/common/log.h"
#include "native/components/aurora/aurora.h"
#include "native/components/aurora/aurora_assets.h"
#include "aurora.h"

using namespace imp;


namespace ix::samsung::homecomponents
{
    // map of texture and mask
    std::tuple<imp::resources::ResourceDefinition, imp::resources::ResourceDefinition> assetsTypes [] =  {
        {assets::kTXAuroraPng, assets::kTXAuroraMaskPng},
        {assets::kTXAurora5Png, assets::kTXAuroraMask5Png},
        {assets::kTXAurora6Png, assets::kTXAuroraMask6Png},
        {assets::kTXAurora8Png, assets::kTXAuroraMask8Png},
        {assets::kTXAurora9Png, assets::kTXAuroraMask9Png},
        {assets::kTXAurora10Png, assets::kTXAuroraMask10Png}
    };

    imp::resources::ResourceDefinition noiseTypes [] =  {
        {assets::kTXNoiseV1Jpg},
        {assets::kTXNoiseV2Jpg},
        {assets::kTXNoiseV3Png},
        {assets::kTXNoiseV2TilePng},
        {assets::kTXNoiseV3TilePng}
    };

    void Aurora::Setup()
    {
        Initialize();
    }
    void Aurora::Setup(AuroraParameters parameters)
    {
        state_.color_1 = parameters.color_1;
        state_.color_2 = parameters.color_2;
        state_.color_3 = parameters.color_3;
        state_.noise_strength = parameters.noise_strength;
        state_.gradient_speed = parameters.gradient_speed;
        state_.vertical_noise_strength = parameters.vertical_noise_strength;
        state_.type = parameters.type;
        Initialize();
    }

    void Aurora::Initialize()
    {
        auto material_future = GetView().GetAssetManager().LoadMaterial(assets::kAuroraMaterialCmat);
        auto noise_texture_future = GetView().GetAssetManager().LoadImage(assets::kTXNoise1Png);
        auto noise2_texture_future = GetView().GetAssetManager().LoadImage(assets::kTXNoise2Png);
        auto noisev_texture_future = GetView().GetAssetManager().LoadImage(noiseTypes[current_noise_type_]);

        material_future.Merge(noise_texture_future, noise2_texture_future, noisev_texture_future)
            .Then([this](std::tuple<AssetPtr<imp::MaterialAsset>, imp::AssetPtr<imp::ImageAsset>, imp::AssetPtr<imp::ImageAsset>, imp::AssetPtr<imp::ImageAsset>> result) {
                auto [material, noise_texture, noise2_texture, noisev_texture] = result;

                imp::TextureFactory::Options options;
                options.min_filter = filament::backend::SamplerMinFilter::NEAREST_MIPMAP_LINEAR;
                options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
                options.wrap_mode = filament::backend::SamplerWrapMode::MIRRORED_REPEAT;

                CreateQuadSettings settings;
                // settings.resolution = 2;
                settings.size = {10, 10};

                auto panelMesh = GetView().GetMeshFactory().CreatePanel(settings);
                auto node = GetNode();

                ComponentHandle<RenderComponent> renderer_model = node->AddComponent<RenderComponent>(RenderComponent::FrustrumCullingMode::kDisabled);
                renderer_model->SetMesh(std::move(panelMesh));

                auto mat = GetView().GetMaterialFactory().CreateMaterial(material);
                mat->SetParameter("Noise1", GetView().GetTextureFactory().CreateTexture(*noise_texture, options));
                mat->SetParameter("Noise2", GetView().GetTextureFactory().CreateTexture(*noise2_texture, options));
                mat->SetParameter("Noisev", GetView().GetTextureFactory().CreateTexture(*noisev_texture, options));
                material_ptr_ = mat.get();

                renderer_model->SetMaterial(std::move(mat));

                OnIsfStateChanged();

            }).KeptBy(this);
    }

    void Aurora::ChangeAssetType()
    {
        if(state_.type == 0 && state_.type == current_type_)
            return;
        current_type_ = state_.type;

        auto aurora_texture_future = GetView().GetAssetManager().LoadImage(std::get<0>(assetsTypes[current_type_ - 1]));
        auto mask_texture_future = GetView().GetAssetManager().LoadImage(std::get<1>(assetsTypes[current_type_ - 1]));

        aurora_texture_future.Merge(mask_texture_future)
            .Then([this](std::tuple<imp::AssetPtr<imp::ImageAsset>, imp::AssetPtr<imp::ImageAsset>> result) mutable {
                auto [aurora_texture, mask_texture] = result;

                CreateQuadSettings settings;
                // settings.resolution = 80;

                auto w = aurora_texture.Get()->GetWidth()*0.005;
                auto h = aurora_texture.Get()->GetHeight()*0.005;

                settings.size = {w, h};

                auto panelMesh = GetView().GetMeshFactory().CreatePanel(settings);

                auto renderer_model = this->GetNode()->GetComponent<RenderComponent>().Get();
                renderer_model->SetMesh(std::move(panelMesh));


                material_ptr_->SetParameter("Mask", GetView().GetTextureFactory().CreateTexture(*mask_texture));
                material_ptr_->SetParameter("Aurora", GetView().GetTextureFactory().CreateTexture(*aurora_texture));
            }).KeptBy(this);
    }

    void Aurora::ChangeNoiseType()
    {
        if(state_.noise_type == current_noise_type_)
            return;
        current_noise_type_ = state_.noise_type;

        auto aurora_texture_future = GetView().GetAssetManager().LoadImage(noiseTypes[current_noise_type_]);

        aurora_texture_future.Then([this](imp::AssetPtr<imp::ImageAsset> noise_texture) mutable {

            imp::TextureFactory::Options options;
            options.min_filter = filament::backend::SamplerMinFilter::NEAREST_MIPMAP_LINEAR;
            options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
            options.wrap_mode = filament::backend::SamplerWrapMode::MIRRORED_REPEAT;

            material_ptr_->SetParameter("Noisev", GetView().GetTextureFactory().CreateTexture(*noise_texture, options));

        }).KeptBy(this);
    }

    void Aurora::OnIsfStateChanged()
    {
        if(material_ptr_ == nullptr)
        {
            IMP_LOG(imp::ERROR) << "invalid material!";
            return;
        }

        Aurora::ChangeAssetType();
        Aurora::ChangeNoiseType();

        material_ptr_->SetParameter("Color1", state_.color_1);
        material_ptr_->SetParameter("Color2", state_.color_2);
        material_ptr_->SetParameter("Color3", state_.color_3);
        material_ptr_->SetParameter("NoiseStrength", state_.noise_strength);
        material_ptr_->SetParameter("GradientSpeed", state_.gradient_speed);
        material_ptr_->SetParameter("VerticalNoiseStrength", state_.vertical_noise_strength);
    }
}
