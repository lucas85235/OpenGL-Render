#ifndef COMPONENTS_STARRY_SKY_NOISE_H
#define COMPONENTS_STARRY_SKY_NOISE_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/starry_sky_noise_state.proto.imp.h"

namespace ix::samsung::homecomponents
{
    struct StarrySkyData
    {
        float star_tex_width    = 1.0;
        float star_tex_height   = 1.0;
        float star_tex_scale    = 4.5;
        float noise_tex_width   = 1.0;
        float noise_tex_height  = 1.0;
        float noise_tex_scale   = 5.00;
        float noise_speed       =-0.04;
        float star_tex_alpha    = 0.4;
        float noise_tex_alpha   = 1.0;
        int   panel_dome        = 19.0;
    };

    class Starry_Sky_Noise : public imp::Component
    {
    private:
        Starry_Sky_NoiseState state_;
        StarrySkyData data;
        StarrySkyData initial_data;
        imp::NodeHandle starry_sky_noise_node_;
        int current_mesh_index_;

    public:
        void Setup();
        void InitializeStarrySkyParameters();
        void InitializeStarrySky();
        void ResetStarrySkyParameters();
        void CreateStarrySky(imp::AssetPtr<imp::MaterialAsset> material, imp::AssetPtr<imp::MaterialAsset> null_material, imp::NodeHandle model_node,
            imp::AssetPtr<imp::ImageAsset> star_tex,imp::AssetPtr<imp::ImageAsset> mask_tex, float star_width, float star_height, float star_scale, float noise_width, float noise_height, float noise_scale,float noise_speed,
            float star_alpha, float noise_alpha, int model_index);

        using IsfInfo = imp::IsfInfo<&Starry_Sky_Noise::state_>;

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };
}

#endif // COMPONENTS_STARRY_SKY_NOISE_H