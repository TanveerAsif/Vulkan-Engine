# Export source and header files for visibility in top-level BUILD
exports_files(["Core.cpp", "include/Core.h"])
# Bazel BUILD file for VulkanCore
cc_library(
    name = "VulkanCore",
    srcs = [
        "Core.cpp",
        "PhysicalDevice.cpp",
        "Queue.cpp",
        "Wrapper.cpp"
    ],
    hdrs = [
        "include/Core.h",
        "include/PhysicalDevice.h",
        "include/Queue.h",
        "include/Wrapper.h"
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    linkopts = ["-lvulkan"],
)
