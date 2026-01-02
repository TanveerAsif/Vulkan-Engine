#ifndef IMGUIRENDERER_H
#define IMGUIRENDERER_H

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Core.h"
namespace VulkanCore
{

class ImGuiRenderer
{
  public:
    ImGuiRenderer(VulkanCore* vulkanCore);
    ~ImGuiRenderer();

    void destroy();

    // called every frame to render ImGui draw data
    // returns a command buffer ready for submission into the graphics queue
    VkCommandBuffer prepareCommandBuffer(uint32_t imageIndex);

  private:
    void createDescriptorPool();
    void initImGui();

    VulkanCore* mVulkanCore;
    int32_t mFramebufferWidth;
    int32_t mFramebufferHeight;

    std::vector<VkCommandBuffer> mCommandBuffers;
    VkDescriptorPool mDescriptorPool;
};

} // namespace VulkanCore

bool isMouseControlledByImGui();

#endif // IMGUIRENDERER_H
