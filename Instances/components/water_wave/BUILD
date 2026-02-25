load("@com_google_impress//build_tools:imp.bzl", "imp_google3_copts", "if_android")
load("@com_google_impress//build_tools:assets.bzl", "imp_assets")
load("@com_google_impress//build_tools:imp_material.bzl", "imp_material")

package(
    default_applicable_licenses = [],
    default_visibility = [
        "//visibility:public",
    ],
    features = [
        "-use_header_modules",  # Incompatible with -std=c++14.
    ],
)

imp_material(
    name = "water_wave_material",
    src = "data/shaders/water_wave.mat",
)

imp_assets(
    name = "assets",
    srcs = [
        # Shaders
        ":water_wave_material",

        # Textures
        "data/textures/TX_Water_Wave.jpeg",
    ],
    namespace = "ix::samsung::homecomponents::assets",
)

cc_library(
    name = "water_wave",
    srcs = ["water_wave.cc"],
    hdrs = ["water_wave.h"],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        ":assets",
        "//proto/components:water_wave_state_proto_cc_imp",
        "//native/components/particle_instances",
        "@com_google_impress//core/lighting:image_based_lighting_asset_iblprefilter_loader"
    ] + if_android([
        "@com_google_impress//core/view/platforms/xr_android:xr",
        "@com_google_impress//core/view/platforms/xr_android:xr_jni",
        "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
    ]),
)
