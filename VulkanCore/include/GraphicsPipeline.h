#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "SimpleMesh.h"

namespace VulkanCore {

    class GraphicsPipeline {
        public:
          GraphicsPipeline(VkDevice device, GLFWwindow *window,
                           VkRenderPass renderPass,
                           VkShaderModule vertShaderModule,
                           VkShaderModule fragShaderModule, SimpleMesh *mesh,
                           int32_t numSwapchainImages);
          ~GraphicsPipeline();
          void bind(VkCommandBuffer commandBuffer);

        private:
            VkDevice mDevice;
            VkPipelineLayout mPipelineLayout;
            VkPipeline mGraphicsPipeline;
    };



} // namespace VulkanCore
