load("@com_google_impress//build_tools:imp.bzl",  "if_desktop", "imp_default_jni_linkopts", "imp_google3_copts", "if_android")
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

imp_material(
    name = "fireflies_materials",
    src = "data/fireflies.mat",
)

imp_assets(
    name = "fireflies_assets",
    srcs = [
        ":fireflies_materials",
    ] + glob(["data/*.png", "data/*.jpg"]),
    namespace = "ix::samsung::assets",
)

cc_library(
    name = "fireflies_effect",
    srcs = ["fireflies.cc"],
    hdrs = ["fireflies.h"],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        ":fireflies_assets",
        "//native/components/particle_system:particle_system",
        "//native/home_components_utils/common:HomeComponentsUtils"
    ],
)
