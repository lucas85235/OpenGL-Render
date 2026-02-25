load("@com_google_impress//build_tools:assets.bzl", "imp_assets")
load("@com_google_impress//build_tools:imp_material.bzl", "imp_material")
load("@com_google_impress//core/proto:imp_isf_data.bzl", "imp_isf_data")

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

imp_isf_data(
    name = "homecomponents_main_isf_data",
    srcs = [
        "main_scene.textproto",
    ],
    deps = [
        "//proto/components:spin_proto"
    ],
)

imp_material(
    name = "hand_normal",
    src = ":hand.mat",
    replacements = {
        "{{blend_mode}}": "transparent",
    },
)

imp_assets(
    name = "home_components_assets",
    srcs = [
        ":homecomponents_main_isf_data",
        "tiger.glb",
        "HotdogDog.glb",
        "Balloon.glb",
        "controller_right.glb",
        "hexagon.glb",
        "Googlehand_L.glb",
        "Googlehand_R.glb",
        ":hand_normal"
    ],
    namespace = "ix::samsung::homecomponents::assets"
)

imp_assets(
    name = "home_support_assets",
    srcs = [
        "//native/data/water_reflection:water_reflection_files",
    ],
    namespace = "ix::samsung::homecomponents::assets"
)
