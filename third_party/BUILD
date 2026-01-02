cc_library(
    name = "stb_image",
    hdrs = ["include/stb_image.h"],
    includes = ["include"],
    visibility = ["//visibility:public"],
)

# ImGui core library
cc_library(
    name = "imgui_core",
    srcs = [
        "ImGui/imgui.cpp",
        "ImGui/imgui_draw.cpp",
        "ImGui/imgui_tables.cpp",
        "ImGui/imgui_widgets.cpp",
    ],
    hdrs = [
        "ImGui/imgui.h",
    ],
    includes = ["ImGui"],
    visibility = ["//visibility:public"],
)

# ImGui GLFW backend
cc_library(
    name = "imgui_glfw",
    srcs = ["ImGui/backend/imgui_impl_glfw.cpp"],
    hdrs = ["ImGui/backend/imgui_impl_glfw.h"],
    deps = [
        ":imgui_core",
    ],
    includes = ["ImGui", "ImGui/backend"],
    visibility = ["//visibility:public"],
)

# ImGui Vulkan backend
cc_library(
    name = "imgui_vulkan",
    srcs = ["ImGui/backend/imgui_impl_vulkan.cpp"],
    hdrs = ["ImGui/backend/imgui_impl_vulkan.h"],
    deps = [
        ":imgui_core",
        "//:vulkan",
    ],
    includes = ["ImGui", "ImGui/backend"],
    visibility = ["//visibility:public"],
)

# Combined ImGui library with both backends
cc_library(
    name = "imgui",
    deps = [
        ":imgui_core",
        ":imgui_glfw",
        ":imgui_vulkan",
    ],
    visibility = ["//visibility:public"],
)
