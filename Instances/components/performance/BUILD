load("@com_google_impress//build_tools:imp.bzl",  "if_desktop", "imp_default_jni_linkopts", "imp_google3_copts", "if_android")

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

cc_library(
    name = "performance",
    srcs = ["performance.cc"],
    hdrs = ["performance.h"],
    deps = [
        "@com_google_impress//:api",
        "@com_google_impress//core/monitor",
    ],
)