load("@com_google_impress//build_tools:imp.bzl",  "if_desktop", "imp_default_jni_linkopts", "imp_google3_copts", "if_android")
load("@com_google_impress//build_tools:assets.bzl", "imp_assets")
load("@com_google_impress//build_tools:imp_material.bzl", "imp_material")

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
    name = "switch_scene",
    srcs = [
        "switch_scene.cc",
    ],
    hdrs = [
        "switch_scene.h",
    ],
    copts = imp_google3_copts(),
    deps = [
        "//native/components/fade:fade",
        "//native/components/skybox:skybox",
        "@com_google_impress//:api",
        "@com_google_impress//core/math:math",
        "@com_google_absl//absl/status",
    ],
)
