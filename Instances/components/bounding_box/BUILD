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

licenses(["unencumbered"])

proto_library(
    name = "bounding_box_state_proto",
    srcs = ["bounding_box_state.proto"],
    deps = [
        "@com_google_impress//core/proto:imp_proto",
    ]
)

imp_cc_proto_library(
    name = "bounding_box_state_imp_cc_proto",
    proto = ":bounding_box_state_proto",
)

imp_material(
    name = "bounds_shader",
    src = ":bounds_shader.mat",
)

imp_assets(
    name = "bounding_box_assets",
    srcs = [
        ":SM_edges_und.glb",
        ":SM_vertex_und.glb",
        ":bounds_shader",
    ],
    namespace = "ix::samsung::homecomponents::data"
)

cc_library(
    name = "bounding_box",
    srcs = [
        "bounding_box.cc",
    ],
    hdrs = [
        "bounding_box.h",
    ],
    deps = [
        "//proto/components:bounding_box_state_proto_cc",
        "//native/components/interaction:interaction",
        "//native/components/timed_spin:timed_spin",
        "//native/components/timer:timer",
        "@com_google_impress//core/line_renderer",
        "@com_google_impress//core/line_renderer:line_renderer_proto_cc",
        "@com_google_impress//:api",
        ":bounding_box_assets"
    ]
)