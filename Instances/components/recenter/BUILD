load("@com_google_impress//build_tools:imp.bzl",  "imp_google3_copts", "if_android")

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
    name = "recenter",
    srcs = ["recenter.cc"],
    hdrs = ["recenter.h"],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
    ] + if_android([
        "@com_google_impress//core/xr:openxr_events",
        "@com_google_impress//core/view/platforms/xr_android:xr",
        "@com_google_impress//core/view/platforms/xr_android:xr_jni",
        "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
    ]),
)
