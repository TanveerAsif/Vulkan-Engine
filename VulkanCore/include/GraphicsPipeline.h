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
                           const SimpleMesh *mesh, int32_t numSwapchainImages,
                           const std::vector<BufferAndMemory> &uniformBuffers,
                           size_t uniformBufferSize);
          ~GraphicsPipeline();
          void bind(VkCommandBuffer commandBuffer, int32_t imageIndex);

        private:
          void createDescriptorSets(
              const SimpleMesh *mesh, int32_t numSwapchainImages,
              const std::vector<BufferAndMemory> &uniformBuffers,
              size_t uniformBufferSize);
          void createDescriptorPool(int32_t numSwapchainImages);
          void createDescriptorSetLayout(
              const std::vector<BufferAndMemory> &uniformBuffers,
              size_t uniformBufferSize);
          void allocateDescriptorSets(int32_t numSwapchainImages);
          void updateDescriptorSets(
              const SimpleMesh *mesh, int32_t numSwapchainImages,
              const std::vector<BufferAndMemory> &uniformBuffers,
              size_t uniformBufferSize);

          VkDevice mDevice;
          VkPipelineLayout mPipelineLayout;
          VkPipeline mGraphicsPipeline;

          VkDescriptorPool mDescriptorPool;
          VkDescriptorSetLayout mDescriptorSetLayout;
          std::vector<VkDescriptorSet> mDescriptorSets;
    };



} // namespace VulkanCore
