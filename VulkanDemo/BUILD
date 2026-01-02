# Bazel BUILD file for VulkanDemo

cc_binary(
    name = "VulkanDemo",

    # TODO : *.cpp and *.h  or include/*.h is not working.
    srcs =  ["main.cpp", "App.cpp", "include/App.h"],
    includes = ["../VulkanCore/include",".", "include"],  # Add "." for local includes
    deps = [
        "//VulkanCore:VulkanCore",
        "@glm//:glm",
        "//third_party:imgui",
    ],
    copts = ["-I/usr/local/include"],  # Use GLFW 3.4 headers
    linkopts = ["/usr/local/lib/libglfw3.a", "-ldl", "-lpthread", "-lX11"],
    data = glob(["shaders/*"]),
)
