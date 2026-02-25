load("@com_google_impress//build_tools:imp.bzl",  "imp_google3_copts")
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
    name = "filament_shadow_state_proto",
    srcs = ["filament_shadow_state.proto"],
    deps = [
        "@com_google_impress//core/proto:imp_proto",
    ]
)

imp_cc_proto_library(
    name = "filament_shadow_state_imp_cc_proto",
    proto = ":filament_shadow_state_proto",
)

cc_library(
    name = "filament_shadow",
    srcs = [
        "filament_shadow.cc",
    ],
    hdrs = [
        "filament_shadow.h",
    ],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        "//proto/components:filament_shadow_state_proto_cc",
    ],
)
