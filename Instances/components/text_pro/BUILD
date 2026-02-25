load("@com_google_impress//build_tools:assets.bzl", "imp_assets")
load("@com_google_impress//build_tools:imp.bzl", "if_android", "if_desktop", "imp_default_jni_linkopts", "imp_google3_copts")
load("@com_google_impress//build_tools:imp_app.bzl", "imp_app")
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

imp_assets(
    name = "text_pro_assets",
    srcs = [
        "//native/components/text_pro/data:noto.png",
        "//native/components/text_pro/data:text_debug.png",
        "//native/components/text_pro/data:text_material",
    ],
    namespace = "ix::samsung::homecomponents::assets",
)

cc_library(
    name = "text_pro",
    srcs = [
        "text_pro.cc",
    ],
    hdrs = [
        "text_pro.h",
    ],
    copts = imp_google3_copts(),
    deps = [
        ":text_pro_assets",
        "//proto/components:text_pro_proto_cc",
        "@com_google_impress//:api",
    ],
)

# This sets up the automated testing and distribution of Imp Sample Apps.
