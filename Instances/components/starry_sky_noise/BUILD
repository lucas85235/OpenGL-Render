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
    name = "starry_sky_noise_material",
    src = "data/shaders/MT_Starry_Sky_Noise.mat",
)

imp_material(
    name = "null_material",
    src = "data/shaders/MT_Null_Material.mat",
)

imp_assets(
    name = "starry_sky_noise_assets",
    srcs = [
        # Models
        "data/models/MDL_Night_250219_v3.glb",

        # Shaders
        ":starry_sky_noise_material",
        ":null_material",

        # Textures
        "data/textures/TX_mask_512x512.png",
        "data/textures/TX_star_texture_2048x1024.png",
    ],
    namespace = "ix::samsung::homecomponents::assets",
)

cc_library(
    name = "starry_sky_noise",
    srcs = [
        "starry_sky_noise.cc",
    ],
    hdrs = [
        "starry_sky_noise.h",
    ],
    deps = [
        ":starry_sky_noise_assets",
        "//proto/components:starry_sky_noise_state_proto_cc_imp",
        "@com_google_impress//:api",
    ],
)
