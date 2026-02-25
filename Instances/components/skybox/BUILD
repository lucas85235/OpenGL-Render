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
    name = "skybox_material",
    src = ":skybox_material.mat",
)

imp_assets(
    name = "skybox_assets",
    srcs = [
        ":skybox_material",
    ],
    namespace = "ix::samsung::homecomponents::data"
)

cc_library(
    name = "skybox",
    srcs = [
        "skybox.cc",
    ],
    hdrs = [
        "skybox.h",
    ],
    copts = imp_google3_copts(),
    deps = [
        ":skybox_assets",
        "@com_google_impress//:api",
        "@com_google_impress//core/lighting:environment_light",
        "@com_google_impress//core/lighting:image_based_lighting_asset",
        "@com_google_impress//core/lighting:image_based_lighting_asset_iblprefilter_loader",
        "@com_google_absl//absl/status",
    ],
)
