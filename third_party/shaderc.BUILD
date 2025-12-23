# BUILD file for shaderc library

cc_library(
    name = "shaderc",
    hdrs = glob([
        "libshaderc/include/**/*.h",
        "libshaderc/include/**/*.hpp",
    ]),
    srcs = glob([
        "libshaderc/src/**/*.cc",
        "libshaderc/src/**/*.h",
    ]),
    includes = [
        "libshaderc/include",
    ],
    deps = [
        "@glslang//:glslang",
        "@spirv_tools//:spirv-tools",
    ],
    visibility = ["//visibility:public"],
    copts = [
        "-std=c++17",
        "-fPIC",
    ],
)
