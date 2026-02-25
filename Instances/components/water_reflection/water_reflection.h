#ifndef COMPONENTS_WATER_REFLECTION_H
#define COMPONENTS_WATER_REFLECTION_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "third_party/filament/filament/include/filament/Texture.h"
#include "proto/components/water_reflection_state.proto.imp.h"

namespace ix::moohan::home_support {

    // Displays a flowing water effect with reflection texture.
    class WaterReflection : public imp::Component {

    private:
        WaterReflectionState state_;
        imp::AssetPtr<imp::MaterialAsset> water_material_asset_;
        imp::AssetPtr<imp::ImageAsset> normalMap_asset_;
        imp::AssetPtr<imp::ImageBasedLightingAsset> reflection_ibl_asset_;
        imp::TexturePtr water_reflection_texture_;
        imp::Material *water_material_ptr_;
        imp::uint2 water_output_texture_size_;
        bool disabled_alpha_;

        imp::ComponentHandle<imp::RenderComponent> water_renderer_;

        imp::Future<imp::TexturePtr> LoadWaterTexture();

        imp::Future<absl::Status> LoadResourceWithIBL(
                imp::Future<imp::AssetPtr<imp::MaterialAsset>> water_material_future,
                imp::Future<imp::AssetPtr<imp::ImageAsset>> normalMap,
                imp::Future<imp::AssetPtr<imp::ImageBasedLightingAsset>> reflection_future);
        //Test Code for using resource package
        imp::Future<absl::Status> LoadResource(
                imp::Future<imp::AssetPtr<imp::MaterialAsset>> water_material_future,
                imp::Future<imp::AssetPtr<imp::ImageAsset>> normalMap,
                imp::Future<imp::TexturePtr> reflection_future);

    public:
        using IsfInfo = imp::IsfInfo<&WaterReflection::state_,
                imp::IsfDependencies<imp::RenderComponent>>;
        imp::Future<absl::Status> Setup();

        void OnIsfStateChanged();
    };

}  // namespace ix::moohan::home_support

#endif // COMPONENTS_WATER_REFLECTION_H