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
    name = "flocking_birds_material",
    src = ":shaders/flocking_birds.mat",
    includes = [
        "//native/home_components_utils/shader_utils:shader_utils",
    ],
)

imp_assets(
    name = "flocking_birds_assets",
    srcs = [
        # Shaders
        ":flocking_birds_material",

        # Textures
        ":textures/TX_Birds.png",
        ":textures/TX_Birds_Left.png",
        ":textures/TX_FlockBirds_inbetween.png",
    ],
    namespace = "ix::samsung::homecomponents::assets",
)

cc_library(
    name = "flocking_birds",
    srcs = ["flocking_birds.cc"],
    hdrs = ["flocking_birds.h"],
    copts = imp_google3_copts(),
    deps = [
        "@com_google_impress//:api",
        ":flocking_birds_assets",
        "//native/components/timer:timer",
        "//proto/components:flocking_birds_state_proto_cc_imp",
        "//native/components/particle_system:particle_system",
        "//native/home_components_utils/common:HomeComponentsUtils",
    ] + if_android([
        "@com_google_impress//core/view/platforms/xr_android:xr",
        "@com_google_impress//core/view/platforms/xr_android:xr_jni",
        "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
    ]),
)
