#include "home_iblprefilter_loader.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include "third_party/filament/libs/iblprefilter/include/filament-iblprefilter/IBLPrefilterContext.h"
#include "third_party/filament/libs/image/include/image/LinearImage.h"
#include "third_party/filament/libs/imageio/include/imageio/ImageDecoder.h"
#include "core/math/vec.h"
#include "core/render/texture_factory.h"

namespace ix::moohan::home_support {

    using namespace imp;

    HomeIblPrefilterLoader::HomeIblPrefilterLoader(std::optional<int2> output_skybox_size,
                                                   std::optional<int2> output_ibl_size,
                                                   uint8_t mipmap_level)
            : output_skybox_size_(output_skybox_size),
              output_ibl_size_(output_ibl_size),
              mipmap_level_(mipmap_level)
    {}

    Future<std::unique_ptr<ImageBasedLightingAsset>>
    HomeIblPrefilterLoader::Load(
            BaseView* view, absl::string_view asset_url,
            Future<resources::Resource> resource_future) {
        return resource_future.Then(
                [output_skybox_size = output_skybox_size_,
                        output_ibl_size = output_ibl_size_,
                        mipmap_level = mipmap_level_,
                        view,
                        url = std::string(asset_url)
                ](resources::Resource resource) -> std::unique_ptr<ImageBasedLightingAsset> {

                    BufferAccess encoded_image = resource.GetData();
                    std::string encoded_image_string(encoded_image.StringView());
                    std::istringstream in_stream(encoded_image_string);

                    // Decode the HDR image with filament ImageDecoder.
                    auto image = std::make_unique<::image::LinearImage>(
                            ::image::ImageDecoder::decode(in_stream, url));
                    uint32_t width = image->getWidth();
                    uint32_t height = image->getHeight();

                    // Create a texture from the decoded image.
                    filament::Texture::PixelBufferDescriptor buffer(
                            image->getPixelRef(),
                            width * height * image->getChannels() * sizeof(float),
                            filament::Texture::Format::RGB, filament::Texture::Type::FLOAT,
                            [](void* buf, size_t, void* data) {
                                // Called after filament finishes uploading the data to the GPU.
                                // Converts the image back into a unique_ptr to destroy it.
                                std::unique_ptr<::image::LinearImage> image(
                                        reinterpret_cast<::image::LinearImage*>(data));
                            },
                            // Release the image, so that it isn't destroyed until it's finished
                            // being uploaded to the gpu.
                            image.release());

                    filament::Texture* equirect_texture = filament::Texture::Builder()
                            .width(width)
                            .height(height)
                            .levels(0xff)
                            .format(filament::Texture::InternalFormat::R11F_G11F_B10F)
                            .sampler(filament::Texture::Sampler::SAMPLER_2D)
                            .build(*view->GetSharedEngine());

                    equirect_texture->setImage(*view->GetSharedEngine(), 0, std::move(buffer));

                    // If there is a custom output size, create a custom output texture.
                    filament::Texture* skybox_texture = nullptr;
                    if (output_skybox_size.has_value()) {
                        skybox_texture = filament::Texture::Builder()
                                .sampler(filament::Texture::Sampler::SAMPLER_CUBEMAP)
                                .format(filament::Texture::InternalFormat::R11F_G11F_B10F)
                                .usage(filament::Texture::Usage::COLOR_ATTACHMENT |
                                       filament::Texture::Usage::SAMPLEABLE)
                                .width(output_skybox_size->x)
                                .height(output_skybox_size->y)
                                .levels(1)
                                .build(*view->GetSharedEngine());
                    }

                    // Convert the texture into cubemaps with IblPrefilter.
                    ::IBLPrefilterContext context(*view->GetSharedEngine());
                    ::IBLPrefilterContext::EquirectangularToCubemap equirectangularToCubemap(context);

                    skybox_texture = equirectangularToCubemap(equirect_texture, skybox_texture);

                    filament::Texture* ibl_texture = nullptr;
                    if (output_ibl_size.has_value()) {
                        filament::Texture* skybox_for_ibl_texture = filament::Texture::Builder()
                                .sampler(filament::Texture::Sampler::SAMPLER_CUBEMAP)
                                .format(filament::Texture::InternalFormat::R11F_G11F_B10F)
                                .usage(filament::Texture::Usage::COLOR_ATTACHMENT |
                                       filament::Texture::Usage::SAMPLEABLE)
                                .width(output_ibl_size->x)
                                .height(output_ibl_size->y)
                                .levels(mipmap_level)
                                .build(*view->GetSharedEngine());
                        skybox_for_ibl_texture = equirectangularToCubemap(equirect_texture, skybox_for_ibl_texture);
                        ::IBLPrefilterContext::SpecularFilter specularFilter(context);
                        ibl_texture = specularFilter(skybox_for_ibl_texture);
                    }

                    view->GetSharedEngine()->destroy(equirect_texture);

                    return std::make_unique<ImageBasedLightingAsset>(
                            nullptr,  //spherical_harmonics
                            ibl_texture? view->GetTextureFactory().WrapTexture(ibl_texture):nullptr, //ibl_cubemap
                            view->GetTextureFactory().WrapTexture(skybox_texture),//skybox_cubemap
                            std::string(url)); //asset url*/
                });
    }

}  // namespace imp
