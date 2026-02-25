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
    name = "default_button",
    src = ":data/SH_DefaultButton.mat",
)

imp_material(
    name = "3d_button",
    src = ":data/SH_3DButton.mat",
)

imp_assets(
    name = "base_button_assets",
    srcs = [
        "data/SM_RoundedCorner.glb",
        "data/SM_RoundedCornerBorder.glb",
        ":default_button",
        ":3d_button"
    ],
    namespace = "ix::samsung::homecomponents::assets",
)

cc_library(
    name = "base_button",
    srcs = ["base_button.cc"],
    hdrs = ["base_button.h"],
    copts = imp_google3_copts(),
    deps = [
        "//native/components/interaction:interaction",
        "@com_google_impress//:api",
        ":base_button_assets"
    ] + if_android([
        "@com_google_impress//core/view/platforms/xr_android:xr",
        "@com_google_impress//core/view/platforms/xr_android:xr_jni",
        "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
    ]),
)