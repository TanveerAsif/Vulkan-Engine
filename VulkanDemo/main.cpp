#include <GL/gl.h>
#include <iostream>
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

int32_t WINDOW_WIDTH = 800;
int32_t WINDOW_HEIGHT = 600;

#include "App.h"

int main()
{
    std::cout << "Vulkan Demo Main Function" << std::endl;

    VulkanApp::App app(WINDOW_WIDTH, WINDOW_HEIGHT);
    app.init("Vulkan App");
    app.run();

    return 0;
}
