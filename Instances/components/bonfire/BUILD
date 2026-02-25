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
    name = "sparks_material",
    src = "data/shaders/MT_Sparks.mat",
    includes = [
        "//native/components/particle_system:utils",
    ],
)

imp_material(
    name = "bonfire_sprite_material",
    src = "data/shaders/MT_BonfireSprite.mat",
)

imp_material(
    name = "bonfire_base_material",
    src = "data/shaders/MT_BonfireBase.mat",
)

imp_material(
    name = "flame_material",
    src = "data/shaders/MT_Flame.mat",
)

imp_material(
    name = "ground_light_material",
    src = "data/shaders/MT_GroundLight.mat"
)

imp_assets(
    name = "bonfire_assets",
    srcs = [
        # Shaders
        ":bonfire_base_material",
        ":bonfire_sprite_material",
        ":flame_material",
        ":sparks_material",
        ":ground_light_material",

        # Textures
        "data/textures/TX_Bonfire1024.png",
        "data/textures/TX_FlameShapeSmooth1.png",
        "data/textures/TX_FlameNoise2.png",
        "data/textures/TX_FlameNoise5.jpg",
        "data/textures/TX_Base_Glow.png",
        "data/textures/TX_Base_Gradient.png",
        "data/textures/TX_SparkParticles.png",
        "data/textures/TX_LightGround.png"
    ],
    namespace = "ix::samsung::homecomponents::assets",
)

cc_library(
    name = "bonfire",
    srcs = ["bonfire.cc"],
    hdrs = ["bonfire.h"],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        ":bonfire_assets",
        "//proto/components:bonfire_state_proto_cc_imp",
        "//native/components/particle_system:particle_system",
    ] + if_android([
        "@com_google_impress//core/view/platforms/xr_android:xr",
        "@com_google_impress//core/view/platforms/xr_android:xr_jni",
        "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
    ]),
)
