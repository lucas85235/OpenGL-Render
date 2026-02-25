load("@com_google_impress//build_tools:imp.bzl", "imp_google3_copts")
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

licenses(["unencumbered"])

imp_material(
    name = "text_material",
    src = ":data/MT_TextButton.mat",
)

imp_assets(
    name = "text_button_assets",
    srcs = [
        ":text_material",
    ],
    namespace = "ix::samsung::homecomponents::assets",
)

cc_library(
    name = "text_button",
    srcs = ["text_button.cc",],
    hdrs = ["text_button.h",],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        "//native/components/base_button",
        "@com_google_impress//core/canvas:canvas_interfaces",
        "@com_google_impress//core/canvas:canvas_source",
        ":text_button_assets"
    ],
)
