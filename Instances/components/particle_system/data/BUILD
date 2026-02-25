load("@com_google_impress//build_tools:imp_material.bzl", "imp_material")
load("@com_google_impress//build_tools:assets.bzl", "imp_assets")

package(
    default_applicable_licenses = [],
    default_visibility = [
        "//visibility:public",
    ],
    features = [
        "-use_header_modules",  # Incompatible with -std=c++14.
    ],
)

licenses(["unencumbered"])

imp_material(
    name = "default_particle",
    src = "default_particle_material.mat",
)

imp_assets(
    name = "particle_system_assets",
    srcs = [
        ":default_particle",
    ] + glob(["*.png"]),
    namespace = "ix::samsung::assets",
)