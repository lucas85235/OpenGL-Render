load("@com_google_impress//build_tools:assets.bzl", "imp_assets")
load("@com_google_impress//build_tools:imp_material.bzl", "imp_material")
load("@com_google_impress//core/proto:imp_isf_data.bzl", "imp_isf_data")
load("@com_google_impress//build_tools:imp_image_based_lighting.bzl", "imp_image_based_lighting")

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

imp_material(
    name = "water_reflection_shader_effect",
    src = "water_reflection_shader_effect.mat",
)

imp_material(
    name = "water_reflection_shader_effect_alpha",
    src = "water_reflection_shader_effect_alpha.mat",
)

filegroup(
    name = "water_reflection_files",
    visibility = ["//native/data:__pkg__"],
    srcs = [
        "water_reflection_shader_effect.cmat",
        "water_reflection_shader_effect_alpha.cmat"
    ] + glob(["*.png"]),
)