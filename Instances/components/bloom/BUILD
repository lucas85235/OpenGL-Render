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

licenses(["unencumbered"])

proto_library(
    name = "bloom_state_proto",
    srcs = ["bloom_state.proto"],
    deps = [
        "@com_google_impress//core/proto:imp_proto",
    ]
)

imp_cc_proto_library(
    name = "bloom_state_imp_cc_proto",
    proto = ":bloom_state_proto",
)

cc_library(
    name = "bloom",
    srcs = [
        "bloom.cc",
    ],
    hdrs = [
        "bloom.h",
    ],
    deps = [
        "//proto/components:bloom_state_proto_cc",
        "@com_google_impress//:api",
    ]
)
