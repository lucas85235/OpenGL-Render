# TODO(b/316892043): This BUILD file is for impress desktop builds only. All production build logic should go in Soong blueprint files.

load("@com_google_impress//build_tools:imp_material.bzl", "imp_material")
load("@com_google_impress//build_tools:assets.bzl", "imp_assets")

package(
    default_applicable_licenses = [],
    default_visibility = [
        "//visibility:public",
    ],
    features = [
        "-use_header_modules",  # Incompatible with -std=c++14.
    ],
)

licenses(["unencumbered"])

filegroup(
    name = "sdf_library",
    srcs = [
        ":sdf.glsl"
    ]
)

imp_material(
    name = "blur_panel",
    src = "blur_panel.mat",
)

imp_material(
    name = "precomposition_android_view",
    src = ":precomposition_android_view.mat",
    includes = [
        ":dataspace_support.glsl",
        ":volume_crop.glsl",
        ":sdf_library",
    ],
)

imp_material(
    name = "highlight",
    src = ":highlight.mat",
    includes = [
        ":round_corner.glsl",
    ],
)


imp_material(
    name = "round_corner",
    src = ":round_corner.mat",
    includes = [
        ":round_corner.glsl",
    ],
)

imp_material(
    name = "glow_boundary",
    src = ":glow_boundary.mat",
    includes = [
        ":glow_boundary.glsl",
    ],
)

imp_material(
    name = "resize_placeholder",
    src = ":resize_placeholder.mat",
    includes = [
        ":sdf.glsl",
    ]
)

imp_material(
    name = "panel_grid",
    src = ":panel_grid.mat",
)

imp_material(
    name = "panel_shadow",
    src = ":panel_shadow.mat",
    includes = [
        ":sdf.glsl",
    ]
)

imp_assets(
    name = "components_assets",
    srcs = [
        ":blur_panel",
        ":precomposition_android_view",
        ":highlight",
        ":panel_grid",
        ":panel_shadow",
        ":resize_placeholder",
        ":round_corner",
        ":glow_boundary",
    ],
    namespace = "ix::samsung::assets",
)