load("@com_google_impress//build_tools:imp.bzl",  "if_desktop", "imp_default_jni_linkopts", "imp_google3_copts", "if_android")
load("@com_google_impress//build_tools:imp_material.bzl", "imp_material")
load("@com_google_impress//build_tools:assets.bzl", "imp_assets")
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
    name = "clouds_material",
    src = "data/shaders/MT_Clouds.mat",
)

imp_assets(
    name = "moving_clouds_assets",
    srcs = [
        # Shaders
        "clouds_material",

        # Texture and models
    ] + glob(["data/textures/*"]) + glob(["data/models/*"]),
    namespace = "ix::samsung::homecomponents::assets"
)

proto_library(
    name = "moving_clouds_state_proto",
    srcs = ["moving_clouds_state.proto"],
    deps = [
        "@com_google_impress//core/proto:imp_proto",
    ]
)

imp_cc_proto_library(
    name = "moving_clouds_state_imp_cc_proto",
    proto = ":moving_clouds_state_proto",
)

cc_library(
    name = "moving_clouds_component",
    srcs = ["moving_clouds_component.cc"],
    hdrs = ["moving_clouds_component.h"],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        ":moving_clouds_assets",
        "//proto/components:moving_clouds_state_proto_cc_imp",
        ":mesh_utils"
    ] + if_android([
        "@com_google_impress//core/view/platforms/xr_android:xr",
        "@com_google_impress//core/view/platforms/xr_android:xr_jni",
        "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
    ]),
)

cc_library(
     name = "mesh_utils",
     srcs = ["MeshUtils.cpp"],
     hdrs = ["MeshUtils.h"],
     copts = imp_google3_copts(),
     deps = [
         "@com_google_impress//:api",
     ] + if_android([
         "@com_google_impress//core/view/platforms/xr_android:xr",
         "@com_google_impress//core/view/platforms/xr_android:xr_jni",
         "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
     ]),
 )