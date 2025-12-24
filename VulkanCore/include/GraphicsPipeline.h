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
                           VkShaderModule fragShaderModule,
                           const SimpleMesh *mesh, int32_t numSwapchainImages);
          ~GraphicsPipeline();
          void bind(VkCommandBuffer commandBuffer, int32_t imageIndex);

        private:
          void createDescriptorSets(const SimpleMesh *mesh,
                                    int32_t numSwapchainImages);
          void createDescriptorPool(int32_t numSwapchainImages);
          void createDescriptorSetLayout();
          void allocateDescriptorSets(int32_t numSwapchainImages);
          void updateDescriptorSets(const SimpleMesh *mesh,
                                    int32_t numSwapchainImages);

          VkDevice mDevice;
          VkPipelineLayout mPipelineLayout;
          VkPipeline mGraphicsPipeline;

          VkDescriptorPool mDescriptorPool;
          VkDescriptorSetLayout mDescriptorSetLayout;
          std::vector<VkDescriptorSet> mDescriptorSets;
    };



} // namespace VulkanCore
