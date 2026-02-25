#ifndef COMPONENTS_SUNSHINE_H
#define COMPONENTS_SUNSHINE_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/sunshine_vfx_state.proto.imp.h"
#include "core/view/framework/render/material.h"
#include "native/components/sunshine_vfx/light_ray.h"

namespace ix::samsung::homecomponents
{
    class SunshineVFXComponent : public imp::Component
    {
    private:
        imp::Material *bloom_material_ptr_;
        imp::Material *ray_large_material_ptr_;
        std::vector<imp::Material*> ray_material_ptrs_;
        std::vector<imp::NodeHandle> ray_nodes_;
        SunshineVFXState state_;
        imp::ComponentHandle<imp::GltfRenderer> ray_renderer_;
        imp::ComponentHandle<imp::GltfRenderer> ray_large_renderer_;
        imp::ComponentHandle<LightRay> rooftop_component_;
        imp::NodeHandle ray_direction_target_;
        imp::NodeHandle bloom_node_;
        imp::NodeHandle rooftop_node_;

        // Material parameter names
        const std::string COLOR = "color";
        const std::string DIRECTION = "direction";
        const std::string BRIGHTNESS = "brightness";
        const std::string TEXTURE = "texture";
        const std::string ALPHA_RANGE = "alphaRange";
        const std::string FADE_SPEED = "fadeSpeed";
        const std::string FADE_OFFSET = "fadeOffset";
        const std::string NOISE_TEXTURE = "noiseTexture";
        const std::string NOISE_COLOR = "noiseColor";
        const std::string NOISE_DIRECTION = "noiseDirection";
        const std::string NOISE_SPEED = "noiseSpeed";
        const std::string NOISE_TILING = "noiseTiling";

        // Material parameters with default values
        imp::float3 ray_direction_ = {0.0, 0.0, 0.0};
        imp::float4 ray_color_ = {0.9, 0.9, 0.7, 1.0};

        bool ray_enabled_ = true;
        float ray_fade_speed_ = 1.5;
        imp::float2 ray_small_alpha_range_ = {0.4, 0.6};
        imp::float2 ray_large_alpha_range_ = {0.7, 1.0};
        imp::float3 ray_direction_offset_;

        imp::float4 noise_color_ = {0.8, 0.8, 0.8, 0.5};
        imp::float2 noise_direction_ = {0.5, 0.5};
        float noise_speed_ = 0.005;
        imp::float2 noise_pattern_tiling_ = {2.0, 2.0};

        bool bloom_enabled_ = false;
        imp::float4 bloom_color_ = {1.0, 1.0, 1.0, 1.0};
        float bloom_brightness_ = 2.0f;
        float bloom_fade_speed_ = 1.5;
        imp::float3 bloom_position_ = {8.738, 4.481, -9.456 };
        imp::float3 bloom_rotation_ = {6.890, 148.694, 324.276 };
        imp::float3 bloom_scale_ = { 1, 1, 1 };
        imp::float2 bloom_alpha_range_ = {0.7, 1.0};

        bool rooftop_enabled_ = true;
        imp::float3 rooftop_plane_position_ = {-2.0, 6.2, -3.8};
        imp::float3 rooftop_plane_rotation_ = {54.0, 90.0, 0.0};


        imp::float3 rooftop_plane_scale_ = {19.557, 27.476, 1};
        imp::float2 rooftop_plane_alpha_range_ = {0.5, 1.0};
        float rooftop_plane_fade_speed_ = 1.0;
        imp::float3 rooftop_color_ = {1.0, 1.0, 1.0};

        // Textures
        std::string ray_pattern_ = "native/components/sunshine_vfx/data/TX_godray_dot_alpha2.png";
        std::string ray_pattern_50_ = "native/components/sunshine_vfx/data/TX_godray_dot_alpha50.png";
        std::string ray_large_pattern_ = "native/components/sunshine_vfx/data/TX_Godlight3.png";
        std::string noise_pattern_ = "native/components/sunshine_vfx/data/TX_Noise512.png";
        std::string rooftop_pattern_ = "native/components/sunshine_vfx/data/TX_godlight_roof.png";

        const float PI = 3.141592;

        void DisableGltfColliders(imp::NodeHandle node);
        void UpdateLocalParameters();
        void UpdateShineDirection();
        void UpdateMaterialParameters();
        void PrintMaterialParameters();
        std::optional<imp::Box> GetBounds(imp::NodeHandle node, std::optional<imp::Box> result);
        imp::Box Union(std::optional<imp::Box> a, const imp::Box &b);
        imp::NodeHandle CreateSmallRay(const imp::AssetPtr<imp::GltfAsset>& gltf, imp::float3 position, imp::float3 rotation,
                                       imp::float3 scale, imp::float2 alpha_range,
                                       const imp::AssetPtr<imp::ImageAsset>& texture,
                                       const imp::AssetPtr<imp::MaterialAsset>& material, const std::string& name);
        imp::NodeHandle CreateBloomNode(imp::MaterialPtr material);

    public:
        void Setup();
        void InitializeVFX();

        using IsfInfo = imp::IsfInfo<&SunshineVFXComponent::state_>;
        void OnIsfStateChanged();

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };
}

#endif // COMPONENTS_SUNSHINE_H
