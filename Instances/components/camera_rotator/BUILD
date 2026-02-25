load("@com_google_impress//build_tools:imp.bzl",  "if_desktop", "imp_default_jni_linkopts", "imp_google3_copts", "if_android")
load("@com_google_impress//build_tools:assets.bzl", "imp_assets")

package(
    default_applicable_licenses = [],
    default_visibility = [
        "//visibility:public",
    ],
    features = [
        "-use_header_modules",  # Incompatible with -std=c++14.
    ],
)

cc_library(
    name = "camera_rotator",
    srcs = [
        "camera_rotator.cc",
    ],
    hdrs = [
        "camera_rotator.h",
    ],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
    ],
)
