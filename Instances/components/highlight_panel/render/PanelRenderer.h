#pragma once

#include <GLES2/gl2.h>

#include "imp.h"

namespace ix::samsung::homecomponents {

// Renders a panel in 3D. A panel is our definition of a quad with 2D content and can be either flat
// or curved.
class PanelRenderer : public imp::Component {
public:
    struct ColorSpaceData {
        // Input color spaces.
        bool isColorSpaceLinearSRGB = true;
        bool isColorSpaceBT2020 = false;

        // Transfer functions.
        bool isTransferLinear = true;
        bool isTransferST2084 = false;

        // Transform that maps RGB in linear input color space to linear ouput color (always linear
        // BT709 in Filament). Only needed if the input color standard is not BT709.
        imp::mat3f colorTransform;
    };

    struct Options {
        // For flat panels, set |curvature| to 0.
        float curvature;
        const imp::float3& curvedPanelScale;
        // If |blurEnabled| is set to true, we use a different shader. This is currently an
        // experimental path and not all features are guaranteed to work there.
        bool blurEnabled = false;

        // isTextureExternal will be set as a specialization constant on that material & cannot be
        // modified as we assume the texture type never changes throughout the life of this texture.
        bool isTextureExternal = false;
        // Color space data will be used as specialization constants on that material & cannot be
        // modified as we assume the color space of a texture never changes throughout the life of
        // this texture.
        ColorSpaceData colorSpaceData;
    };

    // For flat panels, set |curvature| to 0. If |blurEnabled| is set to true, we use a different
    // shader. This is currently an experimental path and not all features are guaranteed to work
    // there.
    imp::Future<absl::Status> Setup(const Options& options);

    void Update(const imp::FrameTime& frameTime);

    // Sets the panel texture from an already existing texture on the GPU. If |colorSpaceData| is
    // not set, there won't be any color space modifications to the incoming textures.
    void SetTexture(GLuint textureId, int widthPixels, int heightPixels);

    // Sets the panel texture from an already existing Impress texture.
    void SetTexture(imp::TexturePtr texture);

    // Sets the global alpha, which is applied uniformly to all fragments. Not supported for blurred
    // panels yet.
    void SetGlobalAlpha(float alpha);

    // Sets the alpha for overlaps, which is separate from global alpha. This alpha is used to
    // help with depth paradoxes when non-active panels are covering active ones.
    void SetOverlapFadeAlpha(float alpha);

    void SetVolumeCrop(imp::float3 halfExtents, imp::mat4f localToCrop);
    void RemoveVolumeCrop();

    // Updates the solid color used. If |solidColor| is nullptr, then it disables the solid color
    // from the shader. Not supported for blurred panels yet.
    void UpdateSolidColor(bool isSolidColorQuad, const imp::float4& solidColor);

    // Converts the uv coordinates from current mesh UVs to new UVs.
    void UpdateUvTransform(const imp::mat4f& uvTransform);

    // Updates the current mesh with new curvature and scale. This should only need to be called for
    // curved panel.
    void UpdateMesh(float curvature, const imp::float3& curvedPanelScale);

    // Returns the axis-aligned bounding box of this panel's mesh.
    const imp::Box& GetAabb() const;

private:
    bool mBlurEnabled;
    bool mIsTextureExternal = false;
    imp::ComponentHandle<imp::RenderComponent> mRender;
    float mCurrOverlapFadeAlpha = 1.0f;
    float mTargetOverlapFadeAlpha = 1.0f;
};

} // namespace ix::samsung::homecomponents
