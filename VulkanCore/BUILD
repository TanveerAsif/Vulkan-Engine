# Export source and header files for visibility in top-level BUILD
exports_files(["Core.cpp", "include/Core.h"])
# Bazel BUILD file for VulkanCore

cc_library(
    name = "vulkan",
    linkopts = [
        "/opt/vulkansdk/1.4.304.1/x86_64/lib/libvulkan.so",
        "-Wl,-rpath,/opt/vulkansdk/1.4.304.1/x86_64/lib",
    ],
    copts = ["-isystem/opt/vulkansdk/1.4.304.1/x86_64/include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "shaderc",
    linkopts = ["-lshaderc_shared"],
    copts = ["-isystem/opt/vulkansdk/1.4.304.1/x86_64/include"],
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
    copts = ["-I/usr/local/include"],  # Use GLFW 3.4 headers
    linkopts = ["/usr/local/lib/libglfw3.a"],  # Use GLFW 3.4 static library
    visibility = ["//visibility:public"],
)
#sudo apt-get install glslang-dev glslang-tools
