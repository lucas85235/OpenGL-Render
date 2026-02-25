#ifndef COMPONENTS_WATER_WAVE_H
#define COMPONENTS_WATER_WAVE_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/water_wave_state.proto.imp.h"
#include "core/view/framework/render/material.h"

namespace ix::samsung::homecomponents
{
    struct WaterWaveParameters
    {
        float sizeOfGrid = 100;
        float velocity = 10.;
        float radius = 0.1;
        float frequency = 10;
        float amplitude = 5;
        const char* textureName = "wallpapers/sna_night/data/sna_night_reflection_map_4K-240527-01.png";
    };

    class WaterWave : public imp::Component
    {
    private:
        imp::Material *material_ptr_;
        WaterWaveState state_;
        imp::AssetPtr<imp::ImageBasedLightingAsset> reflection_ibl_asset_;
        int sizeOfGrid_;        

    public:
        void Setup();
        void Setup(WaterWaveParameters parameters);
        void Initialize();
        void InitializeWithMultiplesShaders();
        void UpdateWaves();

        using IsfInfo = imp::IsfInfo<&WaterWave::state_, imp::IsfDependencies<imp::RenderComponent>>;
        void OnIsfStateChanged();
    };
}

#endif // COMPONENTS_WATER_WAVE_H
