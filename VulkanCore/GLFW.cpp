#include "GLFW.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>

// Use stb_image functions (implementation is in Core.cpp)
#include "stb_image.h"

namespace VulkanCore
{

// Internal GLFW Callbacks
void GLFW_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    GLFWCallbacks* callbacks = static_cast<GLFWCallbacks*>(glfwGetWindowUserPointer(window));
    if (callbacks)
    {
        callbacks->onKeyEvent(window, key, scancode, action, mods);
    }
}

void GLFW_MouseCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    GLFWCallbacks* callbacks = static_cast<GLFWCallbacks*>(glfwGetWindowUserPointer(window));
    if (callbacks)
    {
        callbacks->onMouseMove(window, xoffset, yoffset);
    }
}

void GLFW_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    GLFWCallbacks* callbacks = static_cast<GLFWCallbacks*>(glfwGetWindowUserPointer(window));
    if (callbacks)
    {
        callbacks->onMouseButtonEvent(window, button, action, mods);
    }
}

GLFWwindow* glfw_vulkan_init(int32_t width, int32_t height, const char* title)
{

    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    if (!glfwVulkanSupported())
    {
        throw std::runtime_error("GLFW: Vulkan not supported");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    return window;
}

void glfw_vulkan_set_callbacks(GLFWwindow* window, GLFWCallbacks* callbacks)
{
    glfwSetWindowUserPointer(window, callbacks);

    glfwSetKeyCallback(window, GLFW_KeyCallback);
    glfwSetCursorPosCallback(window, GLFW_MouseCallback);
    glfwSetMouseButtonCallback(window, GLFW_MouseButtonCallback);
}

void glfw_set_window_icon(GLFWwindow* window, const std::string& iconPath)
{
    int width, height, channels;
    unsigned char* pixels = stbi_load(iconPath.c_str(), &width, &height, &channels, 4); // Force RGBA

    if (!pixels)
    {
        std::cerr << "Failed to load window icon: " << iconPath << std::endl;
        return;
    }

    GLFWimage icon;
    icon.width = width;
    icon.height = height;
    icon.pixels = pixels;

    glfwSetWindowIcon(window, 1, &icon);

    stbi_image_free(pixels);
    std::cout << "Window icon set successfully: " << iconPath << std::endl;
}

} // namespace VulkanCore
