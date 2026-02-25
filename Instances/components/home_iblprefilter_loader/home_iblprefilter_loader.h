#ifndef HOME_IBLPREFILTER_LOADER_H_
#define HOME_IBLPREFILTER_LOADER_H_

#include <memory>
#include <optional>

#include "core/lighting/image_based_lighting_asset.h"
#include "core/math/vec.h"

namespace ix::moohan::home_support {

    class HomeIblPrefilterLoader : public imp::ImageBasedLightingAsset::CustomLoader {
    public:
        // Default output size is 256 x 256 if unspecified.
        explicit HomeIblPrefilterLoader(
                std::optional<imp::int2> output_skybox_size = std::nullopt,
                std::optional<imp::int2> output_ibl_size = std::nullopt,
                uint8_t mipmap_level = 0xFF);

        imp::Future<std::unique_ptr<imp::ImageBasedLightingAsset>> Load(
                imp::BaseView* view, absl::string_view asset_url,
                imp::Future<imp::resources::Resource> resource_future) override;

    private:
        // Default output size is 256 x 256 if unspecified.
        std::optional<imp::int2> output_skybox_size_;
        std::optional<imp::int2> output_ibl_size_;
        uint8_t mipmap_level_;
    };

}  // namespace ix::moohan::home_support

#endif  // HOME_IBLPREFILTER_LOADER_H_
