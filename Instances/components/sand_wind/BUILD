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
    name = "sand_wind_layer_material",
    src = "data/shaders/MT_SandWindLayer.mat",
)

imp_material(
    name = "sand_wind_particles_material",
    src = "data/shaders/MT_SandWindParticles.mat",
    includes = [
        "//native/components/particle_system:utils",
    ],
)

imp_assets(
    name = "sand_wind_assets",
    srcs = [
        # Shaders
        ":sand_wind_layer_material",
        ":sand_wind_particles_material",
        # Textures
        "data/textures/TX_Noise_Mask.png",
        "data/textures/TX_Noise_Mask_2.png",
        "data/textures/TX_Noise_Shape1.png",
        "data/textures/TX_Noise_Shape2.png",
        "data/textures/TX_SandWind.png",
        "data/textures/TX_SandWind_Seamless.png",
        "data/textures/TX_SandWind_circle.png",
        "data/textures/TX_Particles_Mask.png",
        "data/textures/TX_Dust_1.png",
        "data/textures/TX_Dust_2.png"
    ],
    namespace = "ix::samsung::homecomponents::assets",
)

cc_library(
    name = "sand_wind",
    srcs = ["sand_wind.cc", "sand_wind_state_machine.cc"],
    hdrs = ["sand_wind.h", "sand_wind_state_machine.h"],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        ":sand_wind_assets",
        "//proto/components:sand_wind_state_proto_cc_imp",
        "//native/components/particle_system:particle_system",
    ] + if_android([
        "@com_google_impress//core/view/platforms/xr_android:xr",
        "@com_google_impress//core/view/platforms/xr_android:xr_jni",
        "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
    ]),
)
