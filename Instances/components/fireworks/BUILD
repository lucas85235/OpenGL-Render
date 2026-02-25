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
    name = "fireworks_material",
    src = "data/shaders/MT_Fireworks.mat",
)


imp_assets(
    name = "fireworks_assets",
    srcs = [
        # Shaders
        ":fireworks_material",

        # Textures
        "data/textures/TX_Square.png"
    ],
    namespace = "ix::samsung::homecomponents::assets",
)


cc_library(
    name = "fireworks",
    srcs = [
        "fireworks.cc",
        "fireworks_manager.cc",
    ],
    hdrs = [
        "fireworks.h",
        "fireworks_manager.h",
    ],
    deps = [
        ":fireworks_assets",
        "//proto/components:fireworks_state_proto_cc_imp",
        "@com_google_impress//:api",
    ]
)
