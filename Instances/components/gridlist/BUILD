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
    name = "gridlist_state_proto",
    srcs = ["gridlist_state.proto"],
    deps = [
        "@com_google_impress//core/proto:imp_proto",
    ]
)

imp_cc_proto_library(
    name = "gridlist_state_imp_cc_proto",
    proto = ":gridlist_state_proto",
)

cc_library(
    name = "gridlist",
    srcs = [
        "gridlist.cc",
    ],
    hdrs = [
        "gridlist.h",
    ],
    deps = [
        "//proto/components:gridlist_state_proto_cc",
        "@com_google_impress//:api",
    ]
)