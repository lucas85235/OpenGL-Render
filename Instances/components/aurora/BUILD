load("@com_google_impress//build_tools:imp.bzl",  "if_desktop", "imp_default_jni_linkopts", "imp_google3_copts", "if_android")
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
    name = "aurora_material",
    src = "data/shaders/Aurora.mat",
)

imp_assets(
    name = "aurora_assets",
    srcs = [
        # Shaders
        ":aurora_material",

        # Textures
        "data/textures/TX_Aurora.png",
        "data/textures/TX_Aurora_5.png",
        "data/textures/TX_Aurora_6.png",
        "data/textures/TX_Aurora_8.png",
        "data/textures/TX_Aurora_9.png",
        "data/textures/TX_Aurora_10.png",
        "data/textures/TX_Aurora_Mask.png",
        "data/textures/TX_Aurora_Mask_5.png",
        "data/textures/TX_Aurora_Mask_6.png",
        "data/textures/TX_Aurora_Mask_8.png",
        "data/textures/TX_Aurora_Mask_9.png",
        "data/textures/TX_Aurora_Mask_10.png",
        "data/textures/TX_Noise_1.png",
        "data/textures/TX_Noise_2.png",
        "data/textures/TX_Noise_v_1.jpg",
        "data/textures/TX_Noise_v_2.jpg",
        "data/textures/TX_Noise_v_2-tile.png",
        "data/textures/TX_Noise_v_3.png",
        "data/textures/TX_Noise_v_3-tile.png",
        "data/textures/Horizontal.png",
    ],
    namespace = "ix::samsung::homecomponents::assets",
)

cc_library(
    name = "aurora",
    srcs = ["aurora.cc"],
    hdrs = ["aurora.h"],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        ":aurora_assets",
        "//proto/components:aurora_state_proto_cc_imp",
    ] + if_android([
        "@com_google_impress//core/view/platforms/xr_android:xr",
        "@com_google_impress//core/view/platforms/xr_android:xr_jni",
        "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
    ]),
)
