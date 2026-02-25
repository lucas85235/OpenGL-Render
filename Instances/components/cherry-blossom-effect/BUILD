load("@com_google_impress//build_tools:assets.bzl", "imp_assets")
load("@com_google_impress//build_tools:imp.bzl", "if_android", "if_desktop", "imp_default_jni_linkopts", "imp_google3_copts")
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
    name = "cherry_blossom",
    src = ":cherry_blossom.mat",
    includes = [
        "flip_book.glsl",
        "getPixel.glsl",
        "position.glsl",
    ],
)

imp_assets(
    name = "cherry_blossom_demo_assets",
    srcs = [
        "data/petals.png",
        "data/petals_2.png",
        "data/petals_3.png",
        ":cherry_blossom",
    ],
    namespace = "ix::samsung::homecomponents::cherry_blossom_demo_data",
)

cc_library(
    name = "cherry-blossom-component",
    srcs = [
        "cherry-blossom-component.cc",
    ],
    hdrs = [
        "cherry-blossom-component.h",
    ],
    copts = imp_google3_copts(),
    deps = [
        ":cherry_blossom_demo_assets",
        "//native/components/particle_instances",
        "//proto/components:cherry_blossom_proto_cc",
        "@com_google_impress//:api",
    ],
)
