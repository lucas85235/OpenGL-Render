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
    name = "water_reflection",
    srcs = [
        "water_reflection.cc",
    ],
    hdrs = [
        "water_reflection.h",
    ],
    copts = imp_google3_copts(),
    deps = [
        "//native/data:home_support_assets",
        "//proto/components:water_reflection_state_proto_cc_imp",
        "//native/components/home_iblprefilter_loader",
        "@com_google_impress//:api",
        "@com_google_impress//core/view/framework",
        "@com_google_impress//third_party/filament/filament:filament_hdrs",
        "@com_google_impress//core/lighting:image_based_lighting_asset_iblprefilter_loader"
    ],
)