load("@com_google_impress//build_tools:imp.bzl",  "imp_default_jni_linkopts", "imp_google3_copts")

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
    name = "home_iblprefilter_loader",
    srcs = [
        "home_iblprefilter_loader.cc",
    ],
    hdrs = [
        "home_iblprefilter_loader.h",
    ],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        "@com_google_impress//third_party/filament/filament:filament_hdrs",
        "@com_google_impress//core/lighting:environment_light",
        "@com_google_impress//core/lighting:image_based_lighting_asset",
        "@com_google_impress//core/lighting:image_based_lighting_asset_iblprefilter_loader"
    ],
    alwayslink = True,
    linkstatic = True,
)