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
    name = "trail_material",
    src = "data/shaders/material.mat",
    includes = [
        "data/shaders/getPixel.glsl",
        "//native/components/particle_system:utils"
    ],
)

imp_assets(
    name = "trail_assets",
    srcs = [
        # Shaders
        ":trail_material",

        # Textures
        "data/textures/texture.png"
    ],
    namespace = "ix::samsung::homecomponents::assets",
)

cc_library(
    name = "trail",
    srcs = [
        "trail.cc",
    ],
    hdrs = [
        "trail.h",
    ],
    deps = [
        ":trail_assets",
        "//native/components/particle_instances",
        "//native/components/particle_system",
        "//native/home_components_utils/common:HomeComponentsUtils",
        "//proto/components:trail_state_proto_cc_imp",
        "@com_google_impress//:api",
    ],
)
