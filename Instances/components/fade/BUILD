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

imp_material(
    name = "color_material",
    src = ":color_material.mat",
)

imp_assets(
    name = "fade_assets",
    srcs = [
        ":color_material",
    ],
    namespace = "ix::samsung::homecomponents::data"
)

cc_library(
    name = "fade",
    srcs = [
        "fade.cc",
    ],
    hdrs = [
        "fade.h",
    ],
    copts = imp_google3_copts(),
    deps = [
        ":fade_assets",
        "@com_google_impress//:api",
        "@com_google_impress//core/math:math",
        "@com_google_absl//absl/status",
    ],
)
