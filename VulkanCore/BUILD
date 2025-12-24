# Export source and header files for visibility in top-level BUILD
exports_files(["Core.cpp", "include/Core.h"])
# Bazel BUILD file for VulkanCore

cc_library(
    name = "vulkan",
    linkopts = ["-lvulkan"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "shaderc",
    linkopts = ["-lshaderc_shared"],
    visibility = ["//visibility:public"],
)


cc_library(
    name = "VulkanCore",
    srcs = [
        "Core.cpp",
        "GLFW.cpp",
        "GraphicsPipeline.cpp",
        "PhysicalDevice.cpp",
        "Queue.cpp",
        "Shader.cpp",
        "SimpleMesh.cpp",
        "Wrapper.cpp",
    ],
    hdrs = glob(["include/*.h"]),
    includes = ["include"],
    deps = [
        ":vulkan",
        ":shaderc",
    ],
    visibility = ["//visibility:public"],
)
#sudo apt-get install glslang-dev glslang-tools
