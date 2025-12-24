#include "GraphicsPipeline.h"
#include <cstdint>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace VulkanCore {

GraphicsPipeline::GraphicsPipeline(
    VkDevice device, GLFWwindow *window, VkRenderPass renderPass,
    VkShaderModule vs, VkShaderModule fs, const SimpleMesh *pMesh,
    int32_t numSwapchainImages,
    const std::vector<BufferAndMemory> &uniformBuffers,
    size_t uniformBufferSize)
    : mDevice(device), mPipelineLayout(VK_NULL_HANDLE),
      mGraphicsPipeline(VK_NULL_HANDLE) {

  if (pMesh) {
    createDescriptorSets(pMesh, numSwapchainImages, uniformBuffers,
                         uniformBufferSize);
  }

  VkPipelineShaderStageCreateInfo shaderStageInfo[2] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .module = vs,
       .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .module = fs,
       .pName = "main"}};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  };

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE};

  int32_t width, height;
  glfwGetFramebufferSize(window, &width, &height);

  VkViewport viewport = {.x = 0.0f,
                         .y = 0.0f,
                         .width = static_cast<float>(width),
                         .height = static_cast<float>(height),
                         .minDepth = 0.0f,
                         .maxDepth = 1.0f};

  VkRect2D scissor = {
      .offset = {0, 0},
      .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}};

  VkPipelineViewportStateCreateInfo viewportState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor};

  VkPipelineRasterizationStateCreateInfo rasterizer = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f};

  VkPipelineMultisampleStateCreateInfo multisampling = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 1.0f};

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {
      .blendEnable = VK_FALSE,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

  VkPipelineColorBlendStateCreateInfo colorBlending = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment};

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

  if (pMesh && pMesh->mVertexBuffer.mBuffer != VK_NULL_HANDLE) {
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &mDescriptorSetLayout;
  } else {
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
  }

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &mPipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create pipeline layout.");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = &shaderStageInfo[0],
      .pVertexInputState = &vertexInputInfo,
      .pInputAssemblyState = &inputAssembly,
      .pViewportState = &viewportState,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pColorBlendState = &colorBlending,
      .layout = mPipelineLayout,
      .renderPass = renderPass,
      .subpass = 0};

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &mGraphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create graphics pipeline.");
  }

  std::cout << "Graphics pipeline created successfully." << std::endl;
}

GraphicsPipeline::~GraphicsPipeline() {
  if (mGraphicsPipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(mDevice, mGraphicsPipeline, nullptr);
  }
  if (mPipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
  }

  if (mDescriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
  }
  if (mDescriptorSetLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
  }
  if (mDescriptorSets.size() > 0) {
    mDescriptorSets.clear();
  }
}

void GraphicsPipeline::bind(VkCommandBuffer commandBuffer, int32_t imageIndex) {
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    mGraphicsPipeline);

  if (mDescriptorSets.size() > 0) {
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mPipelineLayout,
                            0,                            // firstSet
                            1,                            // descriptorSetCount
                            &mDescriptorSets[imageIndex], // pDescriptorSets
                            0,                            // dynamicOffsetCount
                            nullptr);                     // pDynamicOffsets
  }
}

void GraphicsPipeline::createDescriptorSets(
    const SimpleMesh *mesh, int32_t numSwapchainImages,
    const std::vector<BufferAndMemory> &uniformBuffers,
    size_t uniformBufferSize) {
  createDescriptorPool(numSwapchainImages);
  createDescriptorSetLayout(uniformBuffers, uniformBufferSize);
  allocateDescriptorSets(numSwapchainImages);
  updateDescriptorSets(mesh, numSwapchainImages, uniformBuffers,
                       uniformBufferSize);
}

void GraphicsPipeline::createDescriptorPool(int32_t numSwapchainImages) {
  std::vector<VkDescriptorPoolSize> poolSizes;

  // Storage buffer for vertex data
  VkDescriptorPoolSize storageBufferPoolSize = {
      .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = static_cast<uint32_t>(numSwapchainImages),
  };
  poolSizes.push_back(storageBufferPoolSize);

  // Uniform buffer for transformation matrices
  VkDescriptorPoolSize uniformBufferPoolSize = {
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = static_cast<uint32_t>(numSwapchainImages),
  };
  poolSizes.push_back(uniformBufferPoolSize);

  VkDescriptorPoolCreateInfo poolInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = static_cast<uint32_t>(numSwapchainImages),
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data(),
  };

  if (vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mDescriptorPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor pool.");
  }
  std::cout << "Descriptor pool created successfully." << std::endl;
}

void GraphicsPipeline::createDescriptorSetLayout(
    const std::vector<BufferAndMemory> &uniformBuffers,
    size_t uniformBufferSize) {
  std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

  VkDescriptorSetLayoutBinding vertexShaderLayoutBinding_VB = {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
  };
  layoutBindings.push_back(vertexShaderLayoutBinding_VB);

  VkDescriptorSetLayoutBinding uniformBufferLayoutBinding_UBO = {
      .binding = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
  };

  if (uniformBuffers.size() > 0)
    layoutBindings.push_back(uniformBufferLayoutBinding_UBO);

  VkDescriptorSetLayoutCreateInfo layoutInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
      .pBindings = layoutBindings.data(),
  };

  if (vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr,
                                  &mDescriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor set layout.");
  }
  std::cout << "Descriptor set layout created successfully." << std::endl;
}

void GraphicsPipeline::allocateDescriptorSets(int32_t numSwapchainImages) {
  std::vector<VkDescriptorSetLayout> layouts(numSwapchainImages,
                                             mDescriptorSetLayout);

  VkDescriptorSetAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = mDescriptorPool,
      .descriptorSetCount = static_cast<uint32_t>(numSwapchainImages),
      .pSetLayouts = layouts.data(),
  };

  mDescriptorSets.resize(numSwapchainImages);
  if (vkAllocateDescriptorSets(mDevice, &allocInfo, mDescriptorSets.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate descriptor sets.");
  }
  std::cout << "Descriptor sets allocated successfully." << std::endl;
}

void GraphicsPipeline::updateDescriptorSets(
    const SimpleMesh *mesh, int32_t numSwapchainImages,
    const std::vector<BufferAndMemory> &uniformBuffers,
    size_t uniformBufferSize) {
  VkDescriptorBufferInfo bufferInfo_VB = {
      .buffer = mesh->mVertexBuffer.mBuffer,
      .offset = 0,
      .range = mesh->mVertexBufferSize,
  };

  std::vector<VkWriteDescriptorSet> writeDescriptorSets;
  for (int32_t i = 0; i < numSwapchainImages; ++i) {
    VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mDescriptorSets[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &bufferInfo_VB,
    };
    writeDescriptorSets.push_back(descriptorWrite);

    if (uniformBuffers.size() > 0) {
      VkDescriptorBufferInfo bufferInfo_UBO = {
          .buffer = uniformBuffers[i].mBuffer,
          .offset = 0,
          .range = uniformBufferSize,
      };

      VkWriteDescriptorSet descriptorWrite_UBO = {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = mDescriptorSets[i],
          .dstBinding = 1,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo = &bufferInfo_UBO,
      };
      writeDescriptorSets.push_back(descriptorWrite_UBO);
    }
  }
  vkUpdateDescriptorSets(mDevice,
                         static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, nullptr);
  std::cout << "Descriptor sets updated successfully." << std::endl;
}

} // namespace VulkanCore
