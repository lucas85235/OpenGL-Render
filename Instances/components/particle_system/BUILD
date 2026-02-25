load("@com_google_impress//build_tools:assets.bzl", "imp_assets")
load("@com_google_impress//build_tools:imp.bzl", "if_android", "if_desktop", "imp_default_jni_linkopts", "imp_google3_copts")

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

filegroup(
    name = "utils",
    srcs = glob(["utils/*"]),
)

cc_library(
    name = "particle_system",
    srcs = [
        "particle_system.cc",
        "emitter.cc",
        "shape_utils.cc",
    ],
    hdrs = [
        "particle_system.h",
        "particle.h",
        "emitter.h",
        "shape_utils.h",
        "random_value.h",
    ],
    copts = imp_google3_copts(),
    deps = [
        "//proto/components:emitter_state_proto_cc_imp",
        "@com_google_impress//:api",
    ],
)
