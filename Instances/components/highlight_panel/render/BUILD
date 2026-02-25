# TODO(b/316892043): This BUILD file is for impress desktop builds only. All
# production build logic should go in Soong blueprint files.

load(
    "@com_google_impress//build_tools:imp.bzl",
    "imp_google3_copts",
)

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
    name = "panel_renderer",
    srcs = [
        "PanelRenderer.cpp",
    ],
    hdrs = [
        "PanelRenderer.h",
    ],
    copts = imp_google3_copts(),
    deps = [
        "//native/components/highlight_panel/data/materials:components_assets",
        "//samples/sysui_panel_demo:sysui_panel_demo_assets",
        "@com_google_impress//:api",
    ],
)

cc_library(
    name = "highlight_visual",
    srcs = [
        "highlight_visual.cc",
        "highlight_panel.cc",
    ],
    hdrs = [
        "highlight_visual.h",
        "highlight_panel.h",
    ],
    copts = imp_google3_copts(),
    deps = [

        "//native/components/highlight_panel/data/materials:components_assets",
        "@com_google_impress//:api",
        "@com_google_impress//core/animation:property_animator",
        "//proto/components:highlight_visual_state_proto_cc_imp",
    ],
)