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
    name = "snowflake_material",
    src = "data/SH_Snowflake.mat",
    includes = [
        "data/getPixel.glsl",
        "//native/components/particle_system:utils",
    ],
)

imp_material(
    name = "snowflake_material_sphere",
    src = "data/SH_SnowflakeSphere.mat",
    includes = [
        "data/getPixel.glsl",
        "//native/components/particle_system:utils",
    ],
)

imp_assets(
    name = "snowflake_assets",
    srcs = [
        "data/TX_SnowParticle.png",
        "data/TX_SnowParticleSolid.png",
        "data/TX_SnowParticleCap.png",
        "data/TX_SnowParticleSolid50.png",
        "data/TX_Snow_1024.png",
        "data/TX_Snow_512.png",
        ":snowflake_material",
        ":snowflake_material_sphere",
    ],
    namespace = "ix::samsung::homecomponents::assets",
)

cc_library(
    name = "snowflake",
    srcs = [
        "snowflake.cc",
    ],
    hdrs = [
        "snowflake.h",
    ],
    deps = [
        "@com_google_impress//:api",

        "//proto/components:snowflake_state_proto_cc_imp",
        "//native/components/particle_system:particle_system",
        "//native/home_components_utils/common:HomeComponentsUtils",
        "//native/components/particle_instances:particle_instances",

        ":snowflake_assets",
    ] + if_android([
        "@com_google_impress//core/view/platforms/xr_android:xr",
        "@com_google_impress//core/view/platforms/xr_android:xr_jni",
        "@com_google_impress//core/view/platforms/xr_android:xr_action_controller",
    ]),
)