#ifndef COMPONENTS_FADE_H
#define COMPONENTS_FADE_H

#include "imp.h"
#include "core/math/math.h"
#include "native/components/fade/fade_assets.h"

namespace ix::samsung::homecomponents
{
using namespace imp;

struct FadeFinished:Event{
    bool is_fade_in;
};

class Fade : public Component
{
    public:
        void Setup(float time_fade);
        void Update(const FrameTime& frame_time);
        void FadeOut();
        void FadeIn();
        float current = 0;
        bool is_active_fade = false;

    private:
        void FadeController(bool is_fade_in, float initial, float end ,float duration, bool is_active);
        void FadeReset();
        void onFadeFinished();
        NodeHandle AddMaskLocation(std::string prefix, float3 location, AssetPtr<imp::MaterialAsset> material);
        NodeHandle CreateShapeNode(MeshPtr shape_mesh, AssetPtr<imp::MaterialAsset> material);
        float elapsed_, duration_, initial_, end_ = 0;
        bool is_fade_in_ = false;
        int direction_ = 1;
        float const tolerance_ = 0.01f;
        FadeFinished on_fade_finished_;
        Material* material_sphere_;
        NodeHandle mask_node_, camera_node_;
};

}  // namespace ix::samsung::homecomponents

#endif  // COMPONENTS_FADE_H
