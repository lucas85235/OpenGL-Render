load("@com_google_impress//build_tools:imp.bzl",  "if_desktop", "imp_default_jni_linkopts", "imp_google3_copts", "if_android")
load("@com_google_impress//build_tools:assets.bzl", "imp_assets")
load("@com_google_impress//build_tools:imp_material.bzl", "imp_material")
load(
    "@com_google_impress//core/proto:imp_cc_proto_library.bzl",
    "imp_cc_proto_library",
)

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
    name = "light_ray_plane_material",
    src = "material/SH_LightRayPlane.mat",
    includes = [
        "material/utils.glsl"
    ]
)

imp_material(
    name = "light_bloom_with_noise_material",
    src = "material/SH_LightBloomWithNoise.mat",
    includes = [
        "material/utils.glsl"
    ]
)

imp_material(
    name = "textured_model_material",
    src = "material/SH_TexturedModel.mat",
    includes = [
        "material/utils.glsl"
    ]
)

imp_assets(
    name = "sunshine_vfx_assets",
    srcs = [
        "data/LightTEST.glb",
        "data/DM_Sunshine.glb",
        "data/DM_SingleRay.glb",
        "data/TX_Light_Dust_512.png",
        "data/TX_godray_dot_alpha.png",
        "data/TX_godray_dot_alpha2.png",
        "data/TX_godray_dot_alpha50.png",
        "data/TX_Gradient_512.png",
        ":light_ray_plane_material",
        ":light_bloom_with_noise_material",
        "data/SM_Fakebloom.glb",
        "data/SM_Raylarge.glb",
        "data/SM_Raymedium.glb",
        "data/SM_Raysmall.glb",
        "data/TX_Noise512.png",
        "data/TX_Noise2048.png",
        "data/SM_Raylargeuv.glb",
        "data/TX_Ray_Large.png",
        "data/TX_Ray_Large01.png",
        ":textured_model_material",
        "data/SM_Godlight.glb",
        "data/TX_Godlight3.png",
        "data/TX_godlight_roof.png"
    ],
    namespace = "ix::samsung::homecomponents::assets",
)

proto_library(
    name = "sunshine_vfx_state_proto",
    srcs = ["sunshine_vfx_state.proto"],
    deps = [
        "@com_google_impress//core/proto:imp_proto",
    ]
)

imp_cc_proto_library(
    name = "sunshine_vfx_state_imp_cc_proto",
    proto = ":sunshine_vfx_state_proto",
)

cc_library(
    name = "sunshine_vfx_component",
    srcs = ["sunshine_vfx_component.cc"],
    hdrs = ["sunshine_vfx_component.h"],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        ":sunshine_vfx_assets",
        "//proto/components:sunshine_vfx_state_proto_cc_imp",
        ":light_ray"
    ] + if_android([
        "@com_google_impress//core/view/platforms/xr_android:xr",
        "@com_google_impress//core/view/platforms/xr_android:xr_jni",
        "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
    ]),
)

cc_library(
     name = "light_ray",
     srcs = ["light_ray.cc"],
     hdrs = ["light_ray.h"],
     copts = imp_google3_copts(),
     deps = [
         "@com_google_impress//:api",
         ":sunshine_vfx_assets",
     ] + if_android([
         "@com_google_impress//core/view/platforms/xr_android:xr",
         "@com_google_impress//core/view/platforms/xr_android:xr_jni",
         "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
     ]),
 )