#include "native/components/water_reflection/water_reflection.h"

#include "imp.h"

#include <tuple>
#include <utility>

#include "core/lighting/image_based_lighting_asset.h"
#include "core/lighting/image_based_lighting_asset_iblprefilter_loader.h"
#include "third_party/filament/libs/iblprefilter/include/filament-iblprefilter/IBLPrefilterContext.h"
#include "third_party/filament/libs/image/include/image/LinearImage.h"
#include "third_party/filament/libs/imageio/include/imageio/ImageDecoder.h"

#include "native/data/home_support_assets.h"
#include "native/components/home_iblprefilter_loader/home_iblprefilter_loader.h"


namespace ix::moohan::home_support {

    constexpr uint16_t kDefaultWaterReflectionSize = 1024;
    // ex) Step UV : smoothstep(0.0, 0.15, u)*(1.0 - smoothstep(0.85, 1.0, u));
    constexpr imp::float4 KDefaultAlphaStepU = {0.0, 0.15f, 0.85f, 1.0f};
    constexpr imp::float4 KDefaultAlphaStepV = {0.0, 0.15f, 0.85f, 1.0f};
    constexpr float kDefaultAlphaStepMultiplier = 1.0f;

    imp::Future<absl::Status> WaterReflection::Setup() {
        water_renderer_ = GetNode()->GetComponent<imp::RenderComponent>();

        imp::Future<imp::AssetPtr<imp::MaterialAsset>> water_material_future;
        disabled_alpha_ = (state_.disable_alpha.has_value() && state_.disable_alpha.value() == true);
        if (disabled_alpha_) {
            imp::output::Info("[Water_Reflection] : water shader without alpha");
            water_material_future = GetView().GetAssetManager().LoadMaterial(
                    ix::samsung::homecomponents::assets::kWaterReflectionShaderEffectCmat);
        } else {
            imp::output::Info("[Water_Reflection] water shader with alpha");
            water_material_future = GetView().GetAssetManager().LoadMaterial(
                    ix::samsung::homecomponents::assets::kWaterReflectionShaderEffectAlphaCmat);
        }

        imp::Future<imp::AssetPtr<imp::ImageAsset>> normalMap = GetView().GetAssetManager()
                .LoadImage(ix::samsung::homecomponents::assets::kUxwaterPng);

/*        imp::Future<imp::AssetPtr<imp::ImageAsset>> normalMap;
        if (state_.normal_texture.empty()) {
            imp::output::Info("[Water_Reflection] : Normal texture is empty, Load default normal texture!");
            normalMap = GetView().GetAssetManager()
                    .LoadImage(ix::moohan::home_support::assets::kUxwaterPng);
        } else {
            imp::output::Info("[Water_Reflection] : Normal texture is %s", state_.normal_texture);
            normalMap = GetView().GetAssetManager().LoadImage(state_.normal_texture);
            imp::output::Info("[Water_Reflection] : Normal texture 1");

        }*/

        if (state_.water_reflection_size.has_value()) {
            water_output_texture_size_ = state_.water_reflection_size.value();
        } else {
            water_output_texture_size_ = {kDefaultWaterReflectionSize, kDefaultWaterReflectionSize};
        }

        auto waterResourceType = std::string("1");
        if (waterResourceType == "1") {
            auto runtime_ibl_loader = imp::IblPrefilterLoader(water_output_texture_size_);
            imp::Future<imp::AssetPtr<imp::ImageBasedLightingAsset>>
                    reflection_future = GetView().GetAssetManager()
                    .LoadAsset<imp::ImageBasedLightingAsset>(state_.reflection_texture,
                                                             runtime_ibl_loader);
            return LoadResourceWithIBL(water_material_future, normalMap, reflection_future);
        } else {
            auto reflection_future = LoadWaterTexture();
            return LoadResource(water_material_future, normalMap, reflection_future);
        }
    }

    imp::Future<absl::Status> WaterReflection::LoadResourceWithIBL(
            imp::Future<imp::AssetPtr<imp::MaterialAsset>> water_material_future,
            imp::Future<imp::AssetPtr<imp::ImageAsset>> normalMap,
            imp::Future<imp::AssetPtr<imp::ImageBasedLightingAsset>> reflection_future) {

        return water_material_future.Merge(reflection_future, normalMap)
                .Then([this](std::tuple<imp::AssetPtr<imp::MaterialAsset>,
                             imp::AssetPtr<imp::ImageBasedLightingAsset>,
                             imp::AssetPtr<imp::ImageAsset>> result) mutable {

                          auto [water_material_asset, reflection_ibl_asset, normalMap_asset] = result;
                          water_material_asset_  = water_material_asset;
                          reflection_ibl_asset_  = reflection_ibl_asset;
                          normalMap_asset_  = normalMap_asset;
                          imp::MaterialPtr water_material = GetView().GetMaterialFactory().CreateMaterial(water_material_asset_);
                          water_material_ptr_ = water_material.get();
                          water_material->SetName("Water_Reflection");

                          water_material->SetParameter("reflectionCube", reflection_ibl_asset_->GetSkyboxCubemap());
                          water_material->SetParameter("normalMap",
                                                       GetView().GetTextureFactory().CreateTexture(*normalMap_asset, {
                                                               .wrap_mode = imp::TextureFactory::WrapMode::REPEAT,
                                                               .texture_format_override = imp::TextureFactory::Format::RGBA8}));

                          water_renderer_->SetMaterial(std::move(water_material));

                          OnIsfStateChanged();

                          imp::output::Info("[Water_Reflection] Set Water_Reflection (IBL) custom Material!");
                      }
                );
    }

    imp::Future<absl::Status> WaterReflection::LoadResource(imp::Future<imp::AssetPtr<imp::MaterialAsset>> water_material_future,
                                                            imp::Future<imp::AssetPtr<imp::ImageAsset>> normalMap,
                                                            imp::Future<imp::TexturePtr> reflection_future) {

        return water_material_future.Merge(normalMap, reflection_future)
                .Then([this](std::tuple<imp::AssetPtr<imp::MaterialAsset>,
                             imp::AssetPtr<imp::ImageAsset>,
                             imp::TexturePtr> result) mutable {

                    auto water_material_asset = std::get<0>(result);
                    auto normalMap_asset = std::get<1>(result);

                    water_material_asset_  = water_material_asset;
                    imp::MaterialPtr water_material = GetView().GetMaterialFactory().CreateMaterial(water_material_asset_);
                    water_material_ptr_ = water_material.get();
                    normalMap_asset_  = normalMap_asset;
                    water_reflection_texture_ = std::move(std::get<2>(result));

                    water_material->SetName("Water_Reflection");
                    water_material->SetParameter("reflectionCube", water_reflection_texture_.get());
                    water_material->SetParameter("normalMap",
                                                 GetView().GetTextureFactory().CreateTexture(*normalMap_asset, {
                                                         .wrap_mode = imp::TextureFactory::WrapMode::REPEAT,
                                                         .texture_format_override = imp::TextureFactory::Format::RGBA8}));
                    water_renderer_->SetMaterial(std::move(water_material));

                    OnIsfStateChanged();

                    imp::output::Info("Water_Reflection : Set Water_Reflection custom Material!!");
                });
    }

    imp::Future<imp::TexturePtr> WaterReflection::LoadWaterTexture() {
        return GetView().GetAssetManager().LoadResource(state_.reflection_texture)
                .Then([url = state_.reflection_texture](
                        imp::resources::Resource resource)
                              -> std::unique_ptr<::image::LinearImage> {
                    imp::BufferAccess encoded_image = resource.GetData();
                    std::string encoded_image_string(encoded_image.StringView());
                    std::istringstream in_stream(encoded_image_string);

                    // Decode the HDR image with filament ImageDecoder.
                    return std::make_unique<::image::LinearImage>(
                            ::image::ImageDecoder::decode(in_stream, url));
                }, imp::Executor::Type::kBackground)
                .Then([output_texture_size = water_output_texture_size_,
                              view = &GetView()](
                        std::unique_ptr<::image::LinearImage> image)
                              -> imp::TexturePtr {

                    uint32_t width = image->getWidth();
                    uint32_t height = image->getHeight();
                    imp::output::Info("[Water_Reflection] image size : %d, %d, output size :: %d, %d",
                                      width, height, output_texture_size.x, output_texture_size.y);

                    // Create a texture from the decoded image.
                    filament::Texture::PixelBufferDescriptor buffer(
                            image->getPixelRef(),
                            width * height * image->getChannels() * sizeof(float),
                            filament::Texture::Format::RGB, filament::Texture::Type::FLOAT,
                            [](void* buf, size_t, void* data) {
                                std::unique_ptr<::image::LinearImage> image(
                                        reinterpret_cast<::image::LinearImage*>(data));
                            },
                            image.release());

                    filament::Texture* equirect_texture = filament::Texture::Builder()
                            .width(width).height(height).levels(0xff)
                            .format(filament::Texture::InternalFormat::R11F_G11F_B10F)
                            .sampler(filament::Texture::Sampler::SAMPLER_2D)
                            .build(*view->GetSharedEngine());

                    equirect_texture->setImage(*view->GetSharedEngine(), 0, std::move(buffer));

                    filament::Texture* skybox_texture = filament::Texture::Builder()
                            .sampler(filament::Texture::Sampler::SAMPLER_CUBEMAP)
                            .format(filament::Texture::InternalFormat::R11F_G11F_B10F)
                            .usage(filament::Texture::Usage::COLOR_ATTACHMENT |
                                   filament::Texture::Usage::SAMPLEABLE)
                            .width(output_texture_size.x)
                            .height(output_texture_size.y)
                            .levels(0xff)
                            .build(*view->GetSharedEngine());

                    // Convert the texture into cubemaps with IblPrefilter.
                    ::IBLPrefilterContext context(*view->GetSharedEngine());
                    ::IBLPrefilterContext::EquirectangularToCubemap equirectangularToCubemap(context);

                    skybox_texture = equirectangularToCubemap(equirect_texture, skybox_texture);
                    view->GetSharedEngine()->destroy(equirect_texture);
                    return view->GetTextureFactory().WrapTexture(skybox_texture);
                });
    }

    void WaterReflection::OnIsfStateChanged() {
        imp::output::Info("OnIsfStateChanged");
        if (water_material_ptr_ != nullptr) {
            water_material_ptr_->SetParameter("normal_speed", state_.normal_speed);
            water_material_ptr_->SetParameter("normal_tiling", state_.normal_tiling);
            if (!disabled_alpha_) {
                if (state_.alpha_step_u.has_value()) {
                    water_material_ptr_->SetParameter("alpha_step_u",
                                                      state_.alpha_step_u.value());
                } else {
                    water_material_ptr_->SetParameter("alpha_step_u",
                                                      KDefaultAlphaStepU);
                }
                if (state_.alpha_step_v.has_value()) {
                    water_material_ptr_->SetParameter("alpha_step_v",
                                                      state_.alpha_step_v.value());
                } else {
                    water_material_ptr_->SetParameter("alpha_step_v",
                                                      KDefaultAlphaStepV );
                }
                if (state_.alpha_step_multiplier.has_value()) {
                    water_material_ptr_->SetParameter("alpha_step_multiplier",
                                                      state_.alpha_step_multiplier.value());
                } else {
                    water_material_ptr_->SetParameter("alpha_step_multiplier",
                                                      kDefaultAlphaStepMultiplier);
                }
            }
        }
    }
}  // ix::moohan::home_support