load("@com_google_impress//build_tools:assets.bzl", "imp_assets")
load("@com_google_impress//build_tools:imp_material.bzl", "imp_material")
load("@com_google_impress//core/proto:imp_isf_data.bzl", "imp_isf_data")

package(
    default_applicable_licenses = [],
    default_visibility = ["//visibility:public"],
    features = [
        "-use_header_modules",  # Incompatible with -std=c++14.
    ],
)

licenses(["unencumbered"])

exports_files(srcs = [
    "noto.png",
    "text_debug.png",
])

imp_material(
    name = "text_material",
    src = ":text_material.mat",
    includes = [
        ":effectShader.glsl",
        ":textShader.glsl",
    ],
)
