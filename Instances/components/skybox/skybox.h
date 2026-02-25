#ifndef COMPONENTS_SKYBOX_H
#define COMPONENTS_SKYBOX_H

#include "imp.h"
#include "third_party/filament/filament/include/filament/Texture.h"
#include "core/lighting/image_based_lighting_asset_iblprefilter_loader.h"

namespace ix::samsung::homecomponents
{
using namespace imp;

class Skybox : public imp::Component
{
 public:
  void Setup(imp::AssetDefinition asset_definition);
  void Setup(std::vector<resources::ResourceDefinition> background_images);
  void Update(const imp::FrameTime& frame_time);
  void UpdateEnvironmentLight(int index);

 private:
  imp::ComponentHandle<imp::RenderComponent> skybox_renderer_;
  float kDefaultIndirectIntensity = 100.f;
  imp::AssetPtr<imp::ImageBasedLightingAsset> ibl_asset_;
  filament::Texture* texture_ = nullptr;
  void UpdateEnvironmentLight();
  void LoadIblAsset(std::vector<resources::ResourceDefinition> resourceDefinition);
  std::map<int,AssetPtr<ImageBasedLightingAsset>> ibl_assets_;
  int cont_ = 0;
  int ibl_index_ = 0;
};

}  // namespace ix::samsung::homecomponents

#endif  // COMPONENTS_SKYBOX_H
