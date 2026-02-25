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

cc_library(
    name = "hand",
    srcs = [
        "hand.cc",
        "hand_renderer.cc"
    ],
    hdrs = [
        "hand.h",
        "hand_renderer.h"
    ],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        "@com_google_impress//core/xr:openxr_events",
        "@com_google_impress//core/view/platforms/xr_android:xr",
        "//proto/components:hand_renderer_state_proto_cc",
        "//proto/components:hand_state_proto_cc",
        "//native/openxr:hand_tracking",
        "//native/data:home_components_assets",
        "//native/home_components_utils/common:HomeComponentsUtils",
    ],
)
