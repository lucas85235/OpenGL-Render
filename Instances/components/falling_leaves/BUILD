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
    name = "falling_leaves",
    src = "data/falling_leaves.mat",
    includes = [
        "data/getPixel.glsl"
    ],
)

imp_assets(
    name = "falling_leaves_assets",
    srcs = [
        # Materials
        ":falling_leaves",
        "data/TX_Leaf1_part1_v2.png",
        "data/TX_Leaf1_part2_v2.png",
        "data/TX_Leaf2_part1_v2.png",
        "data/TX_Leaf2_part2_v2.png",
        "data/TX_Leaf3_part1_v2.png",
        "data/TX_Leaf3_part2_v2.png",
        "data/TX_LeafAlt_BaW_part1.png",
        "data/TX_LeafAlt_BaW_part2.png"
    ],
    namespace = "ix::samsung::homecomponents::assets"
)

proto_library(
    name = "falling_leaves_state_proto",
    srcs = ["falling_leaves_state.proto"],
    deps = [
        "@com_google_impress//core/proto:imp_proto",
    ]
)

imp_cc_proto_library(
    name = "falling_leaves_state_imp_cc_proto",
    proto = ":falling_leaves_state_proto",
)

cc_library(
    name = "falling_leaves_component",
    srcs = ["falling_leaves_component.cc"],
    hdrs = ["falling_leaves_component.h"],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        ":falling_leaves_assets",
        "//proto/components:falling_leaves_state_proto_cc_imp",
        "//native/components/particle_instances",
    ] + if_android([
        "@com_google_impress//core/view/platforms/xr_android:xr",
        "@com_google_impress//core/view/platforms/xr_android:xr_jni",
        "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
    ]),
)