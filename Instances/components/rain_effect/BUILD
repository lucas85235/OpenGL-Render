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
    name = "rain_material",
    src = "data/rain_effect.mat",
    includes = [
        "data/getPixel.glsl"
    ],
)

imp_assets(
    name = "rain_effect_assets",
    srcs = [
        # Materials
        ":rain_material",
        "data/texture.png",
    ],
    namespace = "ix::samsung::homecomponents::assets"
)

proto_library(
    name = "rain_effect_state_proto",
    srcs = ["rain_effect_state.proto"],
    deps = [
        "@com_google_impress//core/proto:imp_proto",
    ]
)

imp_cc_proto_library(
    name = "rain_effect_state_imp_cc_proto",
    proto = ":rain_effect_state_proto",
)

cc_library(
    name = "rain_effect_component",
    srcs = ["rain_effect_component.cc"],
    hdrs = ["rain_effect_component.h"],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        ":rain_effect_assets",
        "//proto/components:rain_effect_state_proto_cc_imp",
        "//native/components/particle_system",
        "//native/components/particle_instances",
    ] + if_android([
        "@com_google_impress//core/view/platforms/xr_android:xr",
        "@com_google_impress//core/view/platforms/xr_android:xr_jni",
        "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
    ]),
)