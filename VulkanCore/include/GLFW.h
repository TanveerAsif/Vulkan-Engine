#ifndef GLFW_H
#define GLFW_H

#include <cstdint>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace VulkanCore {

class GLFWCallbacks {
public:
  virtual void onKeyEvent(GLFWwindow *window, int key, int scancode, int action,
                          int mods) = 0;
  virtual void onMouseMove(GLFWwindow *window, double xoffset,
                           double yoffset) = 0;
  virtual void onMouseButtonEvent(GLFWwindow *window, int button, int action,
                                  int mods) = 0;
};

// Step1 : initialize GLFW with Vulkan support and create a window
GLFWwindow *glfw_vulkan_init(int32_t width, int32_t height, const char *title);

// Step2 : set GLFW callbacks
void glfw_vulkan_set_callbacks(GLFWwindow *window, GLFWCallbacks *callbacks);

}; // namespace VulkanCore

#endif // GLFW_H
