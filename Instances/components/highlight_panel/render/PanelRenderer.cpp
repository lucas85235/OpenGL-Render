#include "native/components/highlight_panel/render/PanelRenderer.h"
#include "native/components/highlight_panel/data/materials/components_assets.h"

namespace ix::samsung::homecomponents {
namespace {

using MaterialAssetPtr = imp::AssetPtr<imp::MaterialAsset>;

constexpr absl::string_view kShaderParamTexture("androidViewTexture");
constexpr absl::string_view kShaderParamTextureExternal("androidViewTextureExternal");
constexpr absl::string_view kShaderParamUvTransform("uvTransform");
constexpr absl::string_view kShaderParamGlobalAlpha("globalAlpha");
constexpr absl::string_view kShaderParamBlurSampler("blurSampler");
constexpr absl::string_view kBlurTexture("BlurTexture");
constexpr absl::string_view kIsSolidColorQuad("isSolidColorQuad");
constexpr absl::string_view kSolidColor("solidColor");
constexpr absl::string_view kShaderParamOverlapFadeAlpha("overlapFadeAlpha");
constexpr absl::string_view kShaderParamEnableCrop("enableCrop");
constexpr absl::string_view kShaderParamCropHalfExtents("cropHalfExtents");
constexpr absl::string_view kShaderParamTransformLocalToCrop("transformLocalToCrop");

constexpr absl::string_view kShaderConstIsTextureExternal("isExternal");
// Colorspace related constants/parameters.
constexpr absl::string_view kShaderConstIsColorSpaceLinearSRGB("isColorSpaceLinearSRGB");
constexpr absl::string_view kShaderConstIsColorSpaceBT2020("isColorSpaceBT2020");
constexpr absl::string_view kShaderConstIsTransferLinear("isTransferLinear");
constexpr absl::string_view kShaderConstIsTransferST2084("isTransferST2084");
constexpr absl::string_view kColorTransform("colorTransform");

// TODO(perk): Move to state proto so we can tweak these at runtime.
constexpr float kPanelMeshCornerRadius = 0.05f;
constexpr int kPanelMeshResolution = 100;
constexpr float kOverlapAlphaLerpFactor = 3.0f;

imp::MeshPtr CreatePanelMesh(imp::MeshFactory& meshFactory, const float curvature,
                             const imp::float3& curvedPanelScale) {
    const imp::CreateQuadSettings settings{
            .flip_uv = true,
            .radius = imp::AlmostEqual(curvature, 0.0f) ? 0.f : 1.0 / curvature,
            .corner_radius = kPanelMeshCornerRadius,
            .resolution = kPanelMeshResolution,
    };
    return meshFactory.CreatePanel(settings, curvedPanelScale);
}

imp::TexturePtr CreateTexture(imp::TextureFactory& textureFactory, const GLuint textureId,
                              const int widthPixels, const int heightPixels,
                              const bool bufferExternal) {
    if (bufferExternal) {
        // Render external image sources (e.g. videos streaming)
        return textureFactory.CreateExternalTexture(textureId);
    }
    // Render internal images
    const imp::TextureFactory::Format format = filament::Texture::InternalFormat::RGBA8;
    const imp::TextureFactory::Usage usage =
            filament::Texture::Usage::COLOR_ATTACHMENT | filament::Texture::Usage::SAMPLEABLE;
    return textureFactory.CreateTexture(textureId, widthPixels, heightPixels, 1, format, usage);
}

} // namespace

imp::Future<absl::Status> PanelRenderer::Setup(const Options& options) {
    mBlurEnabled = options.blurEnabled;
    mIsTextureExternal = options.isTextureExternal;
    auto onMaterialLoaded = [this, options](const MaterialAssetPtr& materialAsset) {
        mRender = GetNode()->GetOrAddComponent<imp::RenderComponent>();
        imp::MeshPtr mesh = CreatePanelMesh(GetView().GetMeshFactory(), options.curvature,
                                            options.curvedPanelScale);
        mRender->SetMesh(std::move(mesh));

        imp::MaterialPtr materialPtr = GetView().GetMaterialFactory().CreateMaterial(materialAsset);
        mRender->SetMaterial(std::move(materialPtr));
        if (mBlurEnabled) {
            imp::Texture* blurTexture = GetView().GetTextureRegistry().GetTexture(kBlurTexture);
            mRender->GetMaterial()->SetParameter(kShaderParamBlurSampler, blurTexture);
        }
        mRender->GetMaterial()->SetParameter(kColorTransform,
                                             options.colorSpaceData.colorTransform);
        SetGlobalAlpha(1.0f);
        UpdateUvTransform(imp::kIdentityMat4f);
        return absl::OkStatus();
    };
    const auto materialResourceDef = mBlurEnabled
            ? assets::kBlurPanelCmat
            : assets::kPrecompositionAndroidViewCmat;

    imp::MaterialPreCompileOptions mat_options;
    mat_options.constants.push_back(
            {.name = std::string(kShaderConstIsTextureExternal), .value = mIsTextureExternal});
    mat_options.constants.push_back({.name = std::string(kShaderConstIsColorSpaceLinearSRGB),
                                     .value = options.colorSpaceData.isColorSpaceLinearSRGB});
    mat_options.constants.push_back({.name = std::string(kShaderConstIsColorSpaceBT2020),
                                     .value = options.colorSpaceData.isColorSpaceBT2020});
    mat_options.constants.push_back({.name = std::string(kShaderConstIsTransferLinear),
                                     .value = options.colorSpaceData.isTransferLinear});
    mat_options.constants.push_back({.name = std::string(kShaderConstIsTransferST2084),
                                     .value = options.colorSpaceData.isTransferST2084});
    imp::AssetManager& assetManager = GetView().GetAssetManager();
    return assetManager.LoadMaterial(materialResourceDef, mat_options)
            .Then(std::move(onMaterialLoaded));
}

void PanelRenderer::Update(const imp::FrameTime& frameTime) {
    mCurrOverlapFadeAlpha = imp::lerp(mCurrOverlapFadeAlpha, mTargetOverlapFadeAlpha,
                                      frameTime.GetDeltaSeconds() * kOverlapAlphaLerpFactor);
    mRender->GetMaterial()->SetParameter(kShaderParamOverlapFadeAlpha, mCurrOverlapFadeAlpha);
}

void PanelRenderer::SetTexture(const GLuint textureId, const int widthPixels,
                               const int heightPixels) {
    imp::TextureFactory& textureFactory = GetView().GetTextureFactory();
    imp::TexturePtr texture =
            CreateTexture(textureFactory, textureId, widthPixels, heightPixels, mIsTextureExternal);
    auto shaderTextureParam =
            mIsTextureExternal ? kShaderParamTextureExternal : kShaderParamTexture;
    mRender->GetMaterial()->SetParameter(shaderTextureParam, std::move(texture));
}

void PanelRenderer::SetTexture(imp::TexturePtr texture) {
    mRender->GetMaterial()->SetParameter(kShaderParamTexture, std::move(texture));
}

void PanelRenderer::SetGlobalAlpha(float alpha) {
    if (mBlurEnabled) {
        return;
    }
    mRender->GetMaterial()->SetParameter(kShaderParamGlobalAlpha, alpha);
}

void PanelRenderer::SetOverlapFadeAlpha(float alpha) {
    if (mBlurEnabled) {
        return;
    }
    mTargetOverlapFadeAlpha = alpha;
}

void PanelRenderer::SetVolumeCrop(imp::float3 halfExtents, imp::mat4f localToCrop) {
    mRender->GetMaterial()->SetParameter(kShaderParamEnableCrop, true);
    mRender->GetMaterial()->SetParameter(kShaderParamCropHalfExtents, halfExtents);
    mRender->GetMaterial()->SetParameter(kShaderParamTransformLocalToCrop, localToCrop);
}

void PanelRenderer::RemoveVolumeCrop() {
    mRender->GetMaterial()->SetParameter(kShaderParamEnableCrop, false);
}

void PanelRenderer::UpdateSolidColor(bool isSolidColorQuad, const imp::float4& solidColor) {
    if (mBlurEnabled) {
        return;
    }
    mRender->GetMaterial()->SetParameter(kIsSolidColorQuad, isSolidColorQuad);
    if (isSolidColorQuad) {
        mRender->GetMaterial()->SetParameter(kSolidColor, solidColor);
    }
}

void PanelRenderer::UpdateUvTransform(const imp::mat4f& uvTransform) {
    mRender->GetMaterial()->SetParameter(kShaderParamUvTransform, uvTransform);
}

void PanelRenderer::UpdateMesh(const float curvature, const imp::float3& curvedPanelScale) {
    imp::MeshPtr mesh = CreatePanelMesh(GetView().GetMeshFactory(), curvature, curvedPanelScale);
    mRender->SetMesh(std::move(mesh));
}

const imp::Box& PanelRenderer::GetAabb() const {
    return mRender->GetMesh()->GetAabb();
}

} // namespace ix::samsung::homecomponents
