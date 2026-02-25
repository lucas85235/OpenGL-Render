#include "skybox.h"

#include "imp.h"
#include "native/components/skybox/skybox_assets.h"

using namespace imp;

namespace ix::samsung::homecomponents {

// Name of the parameter for the skybox cubemap texture in the skybox material.
constexpr absl::string_view kSkyboxParameter = "skybox";

// Rendering priority for the skybox is zero which means render first (behind
// everything else).
constexpr uint8_t kSkyboxPriority = 0;

void Skybox::Setup(std::vector<resources::ResourceDefinition> background_images) {
    GetView().GetAssetManager().LoadMaterial(data::kSkyboxMaterialCmat)
            .Then([this, background_images](AssetPtr<MaterialAsset> material) mutable {
                // Create the RenderComponent used to render the skybox.
                skybox_renderer_ = GetNode()->AddComponent<RenderComponent>(
                        RenderComponent::FrustumCullingMode::kDisabled);
                skybox_renderer_->SetMesh(
                        GetView().GetMeshFactory().CreateQuad());
                skybox_renderer_->SetShadowCastingMode(
                        RenderComponent::ShadowMode::kNone);
                skybox_renderer_->SetShadowReceivingMode(
                        RenderComponent::ShadowMode::kNone);
                skybox_renderer_->SetPriority(kSkyboxPriority);
                // Assign the skybox material to the RenderComponent.
                skybox_renderer_->SetMaterial(
                        GetView().GetMaterialFactory().CreateMaterial(material));
                // Disable until we have a reflections texture.
                skybox_renderer_->SetEnabled(false);
                LoadIblAsset(background_images);
    }).KeptBy(this);
}

void Skybox::Setup(imp::AssetDefinition asset_definition) {
  auto self = GetHandle(this);

  GetView().GetAssetManager().LoadMaterial(data::kSkyboxMaterialCmat)
    .Then([self](AssetPtr<MaterialAsset> material) mutable {
        // Create the RenderComponent used to render the skybox.
        self->skybox_renderer_ = self->GetNode()->AddComponent<RenderComponent>(
            RenderComponent::FrustumCullingMode::kDisabled);
        self->skybox_renderer_->SetMesh(
            self->GetView().GetMeshFactory().CreateQuad());
        self->skybox_renderer_->SetShadowCastingMode(
            RenderComponent::ShadowMode::kNone);
        self->skybox_renderer_->SetShadowReceivingMode(
            RenderComponent::ShadowMode::kNone);
        self->skybox_renderer_->SetPriority(kSkyboxPriority);

        // Assign the skybox material to the RenderComponent.
        self->skybox_renderer_->SetMaterial(
            self->GetView().GetMaterialFactory().CreateMaterial(material));

        // Disable until we have a reflections texture.
        self->skybox_renderer_->SetEnabled(false);
    }).KeptBy(this);

    GetView().GetAssetManager().LoadImageBasedLighting(asset_definition)
        .Then([this](AssetPtr <ImageBasedLightingAsset> ibl_asset) {
            ibl_asset_ = std::move(ibl_asset);
            UpdateEnvironmentLight();
        }).KeptBy(this);
}

void Skybox::Update(const FrameTime& frame_time) {
  // Poll each frame to see if the reflections texture has changed.
  // There is no event/future based API to get this at the moment.
  const EnvironmentLight* environment_light =
      GetView().GetLightManager().GetEnvironmentLight();
  filament::Texture* texture = nullptr;
  if (environment_light) {
    if ((*environment_light->GetReflectionIblAsset())->GetSkyboxCubemap()) {
      texture = (*environment_light->GetReflectionIblAsset())
                                ->GetSkyboxCubemap()
                                ->GetTexture();
    } else if ((*environment_light->GetReflectionIblAsset())
                   ->GetLightingCubemap()) {
      texture = (*environment_light->GetReflectionIblAsset())
                                ->GetLightingCubemap()
                                ->GetTexture();
    }
    else {
      texture = nullptr;
    }
  }

  if (texture != texture_ && skybox_renderer_) {
    texture_ = texture;

    // Set the texture on the material.
    // Doing it through the filament material instance directly because light
    // manager doesn't provide an Impress texture wrapper.
    skybox_renderer_->GetMaterial()
        ->GetFilamentMaterialInstance()
        ->setParameter(
            kSkyboxParameter.data(), texture,
            filament::TextureSampler(
                filament::TextureSampler::MinFilter::LINEAR_MIPMAP_LINEAR,
                filament::TextureSampler::MagFilter::LINEAR));

    // Enable showing the skybox if the reflections texture exists.
    skybox_renderer_->SetEnabled(texture_ != nullptr);
  }
}

void Skybox::UpdateEnvironmentLight() {
    // Creates EnvironmentLight from IBL asset.
    EnvironmentLightPtr environment_light_ptr = GetView().GetEnvironmentLightFactory()
        .CreateEnvironmentLight(ibl_asset_, kDefaultIndirectIntensity);
    // Set EnvironmentLight in LightManager.
    GetView().GetLightManager().SetEnvironmentLight(std::move(environment_light_ptr));
}

void Skybox::UpdateEnvironmentLight(int index) {
    // Creates EnvironmentLight from IBL asset.
    EnvironmentLightPtr environment_light_ptr =
            GetView().GetEnvironmentLightFactory().CreateEnvironmentLight(
                    ibl_assets_[index], kDefaultIndirectIntensity);

    // Set EnvironmentLight in LightManager.
    GetView().GetLightManager().SetEnvironmentLight(std::move(environment_light_ptr));
}

void Skybox::LoadIblAsset(std::vector<resources::ResourceDefinition> resourceDefinition) {
    GetView().GetAssetManager().LoadAsset<ImageBasedLightingAsset>(resourceDefinition[cont_])
            .Then([=](AssetPtr<ImageBasedLightingAsset> ibl_asset) {
                ibl_assets_[cont_] = std::move(ibl_asset);
                cont_++;
                if (cont_ < 8) {
                    if (cont_ == 1) {
                        UpdateEnvironmentLight(0);
                    }
                    LoadIblAsset(resourceDefinition);
                }

    }).KeptBy(this);
}

}  // namespace ix::samsung::homecomponents
