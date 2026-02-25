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
    name = "icon_material",
    src = "data/MT_IconButton.mat",
)

imp_assets(
    name = "icon_button_assets",
    srcs = [
        ":icon_material",
    ],
    namespace = "ix::samsung::homecomponents::assets",
)

cc_library(
    name = "icon_button",
    srcs = [
        "icon_button.cc",
    ],
    hdrs = [
        "icon_button.h",
    ],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        "//native/components/base_button",
        ":icon_button_assets"
    ],
)
