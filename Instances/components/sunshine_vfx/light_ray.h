#ifndef COMPONENTS_LIGHT_RAY_H
#define COMPONENTS_LIGHT_RAY_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "absl/status/status.h"

namespace ix::samsung::homecomponents
{
    class LightRay : public imp::Component {

    private:
        imp::Material* material_;
        imp::AssetPtr<imp::ImageAsset> texture_asset_;
        imp::AssetPtr<imp::MaterialAsset> material_asset_;
        imp::float3 color_;
        imp::float2 alpha_range_;
        float fade_speed_;
        std::string name_;

    public:
        void Setup();
        void Setup(std::string name,
                   imp::AssetPtr<imp::ImageAsset> texture_asset,
                   imp::AssetPtr<imp::MaterialAsset> material_asset,
                   imp::float2 alpha_range,
                   float fade_speed,
                   imp::float3 color);
        imp::NodeHandle CreateProceduralQuad(imp::float2 size, imp::float2 center);
        imp::NodeHandle CreateFromGltfAsset(imp::AssetPtr<imp::GltfAsset> gltf);
        void SetColor(imp::float3 color);
        void SetPlaneFadeSpeed(float fade_speed);
    };
}
#endif //COMPONENTS_LIGHT_RAY_H
