# Export source and header files for visibility in top-level BUILD
exports_files(["Core.cpp", "include/Core.h"])
# Bazel BUILD file for VulkanCore

cc_library(
    name = "vulkan",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "shaderc",
    linkopts = ["-lshaderc_shared"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "assimp",
    linkopts = [
        "-L/usr/local/lib",
        "-lassimp",
    ],
    visibility = ["//visibility:public"],
)


cc_library(
    name = "VulkanCore",
    srcs = [
        "Camera.cpp",
        "Core.cpp",
        "GLFW.cpp",
        "GraphicsPipeline.cpp",
        "GraphicsPipelineV2.cpp",
        "ImGuiRenderer.cpp",
        "PhysicalDevice.cpp",
        "model/Material.cpp",
        "model/Mesh.cpp",
        "model/Model.cpp",
        "Queue.cpp",
        "Shader.cpp",
        "SkyBox.cpp",
        "SimpleMesh.cpp",
        "Texture.cpp",
        "Wrapper.cpp",
        "VulkanModel.cpp",
    ],
    hdrs = glob(["include/*.h"]) + glob(["model/include/*.h"]),
    includes = ["include", "model/include"],
    deps = [
        ":vulkan",
        ":shaderc",
        ":assimp",
        "//third_party:stb_image",
        "//third_party:imgui",
    ],
    visibility = ["//visibility:public"],
)
#sudo apt-get install glslang-dev glslang-tools
