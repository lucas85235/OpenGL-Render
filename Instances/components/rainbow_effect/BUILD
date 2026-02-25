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
    name = "rainbow_effect",
    src = "data/rainbow_effect.mat"
)

imp_assets(
    name = "rainbow_effect_assets",
    srcs = [
        # Materials
        ":rainbow_effect",
        "data/TX_Rainbow_2048.png",
    ],
    namespace = "ix::samsung::homecomponents::assets"
)

cc_library(
    name = "rainbow_effect_component",
    srcs = ["rainbow_effect_component.cc"],
    hdrs = ["rainbow_effect_component.h"],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        ":rainbow_effect_assets",
        "//proto/components:rainbow_effect_state_proto_cc_imp"
    ] + if_android([
        "@com_google_impress//core/view/platforms/xr_android:xr",
        "@com_google_impress//core/view/platforms/xr_android:xr_jni",
        "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
    ]),
)