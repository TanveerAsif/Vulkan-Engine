#include "GraphicsPipelineV2.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace VulkanCore
{

GraphicsPipelineV2::GraphicsPipelineV2(VkDevice device, GLFWwindow* window, VkRenderPass renderPass,
                                       VkShaderModule vsModule, VkShaderModule fsModule, int32_t numImages,
                                       VkFormat colorFormat, VkFormat depthFormat)
    : mDevice(device), mGraphicsPipeline(VK_NULL_HANDLE), mPipelineLayout(VK_NULL_HANDLE),
      mDescriptorPool(VK_NULL_HANDLE), mDescriptorSetLayout(VK_NULL_HANDLE), mNumImages(numImages)
{
    createDescriptorSetLayout(true, true, true, true, false); // VB, IB, Uniform, Tex2D, Cubemap
    initCommon(window, renderPass, vsModule, fsModule, numImages, colorFormat, depthFormat, VK_COMPARE_OP_LESS);
}

GraphicsPipelineV2::~GraphicsPipelineV2()
{
    vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
    vkDestroyPipeline(mDevice, mGraphicsPipeline, nullptr);
}

void GraphicsPipelineV2::bind(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);
}

void GraphicsPipelineV2::allocateDescriptorSets(int32_t numSubmeshes,
                                                std::vector<std::vector<VkDescriptorSet>>& descriptorSets)
{
    createDescriptorPool(numSubmeshes * mNumImages);
    allocateDescriptorSetsInternal(numSubmeshes, descriptorSets);
}

void GraphicsPipelineV2::updateDescriptorSets(const ModelDesc& modelDesc,
                                              std::vector<std::vector<VkDescriptorSet>>& descriptorSets)
{
    int32_t numSubmeshes = static_cast<int32_t>(descriptorSets[0].size());

    std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    std::vector<VkDescriptorBufferInfo> BufferInfo_VBs(numSubmeshes);
    std::vector<VkDescriptorBufferInfo> BufferInfo_IBs(numSubmeshes);
    std::vector<std::vector<VkDescriptorBufferInfo>> BufferInfo_Uniforms(mNumImages);
    std::vector<VkDescriptorImageInfo> ImageInfo(numSubmeshes);

    // Prepare buffer and image infos
    for (int32_t submeshIndex = 0; submeshIndex < numSubmeshes; submeshIndex++)
    {
        BufferInfo_VBs[submeshIndex].buffer = modelDesc.mVertexBuffer;
        BufferInfo_VBs[submeshIndex].offset = modelDesc.mRanges[submeshIndex].mVbRange.mOffset;
        BufferInfo_VBs[submeshIndex].range = modelDesc.mRanges[submeshIndex].mVbRange.mRange;

        BufferInfo_IBs[submeshIndex].buffer = modelDesc.mIndexBuffer;
        BufferInfo_IBs[submeshIndex].offset = modelDesc.mRanges[submeshIndex].mIbRange.mOffset;
        BufferInfo_IBs[submeshIndex].range = modelDesc.mRanges[submeshIndex].mIbRange.mRange;

        ImageInfo[submeshIndex].sampler = modelDesc.mMaterials[submeshIndex].mSampler;
        ImageInfo[submeshIndex].imageView = modelDesc.mMaterials[submeshIndex].mImageView;
        ImageInfo[submeshIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // Create descriptor writes only for valid resources
    for (int32_t imageIndex{0}; imageIndex < mNumImages; ++imageIndex)
    {
        BufferInfo_Uniforms[imageIndex].resize(numSubmeshes);
        for (int32_t submeshIndex = 0; submeshIndex < numSubmeshes; submeshIndex++)
        {
            BufferInfo_Uniforms[imageIndex][submeshIndex].buffer = modelDesc.mUniformBuffers[imageIndex];
            BufferInfo_Uniforms[imageIndex][submeshIndex].offset =
                modelDesc.mRanges[submeshIndex].mUniformRange.mOffset;
            BufferInfo_Uniforms[imageIndex][submeshIndex].range = modelDesc.mRanges[submeshIndex].mUniformRange.mRange;

            // VB - always valid
            if (BufferInfo_VBs[submeshIndex].buffer != VK_NULL_HANDLE)
            {
                writeDescriptorSets.push_back({
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = descriptorSets[imageIndex][submeshIndex],
                    .dstBinding = V2_BindingVB,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &BufferInfo_VBs[submeshIndex],
                });
            }

            // IB - always valid
            if (BufferInfo_IBs[submeshIndex].buffer != VK_NULL_HANDLE)
            {
                writeDescriptorSets.push_back({
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = descriptorSets[imageIndex][submeshIndex],
                    .dstBinding = V2_BindingIB,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &BufferInfo_IBs[submeshIndex],
                });
            }

            // Uniform - always valid
            if (BufferInfo_Uniforms[imageIndex][submeshIndex].buffer != VK_NULL_HANDLE)
            {
                writeDescriptorSets.push_back({
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = descriptorSets[imageIndex][submeshIndex],
                    .dstBinding = V2_BindingUniform,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &BufferInfo_Uniforms[imageIndex][submeshIndex],
                });
            }

            // Tex2D - only if texture exists
            if (ImageInfo[submeshIndex].imageView != VK_NULL_HANDLE &&
                ImageInfo[submeshIndex].sampler != VK_NULL_HANDLE)
            {
                writeDescriptorSets.push_back({
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = descriptorSets[imageIndex][submeshIndex],
                    .dstBinding = V2_BindingTexture2D,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &ImageInfo[submeshIndex],
                });
            }
        }
    }

    if (!writeDescriptorSets.empty())
    {
        vkUpdateDescriptorSets(mDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(),
                               0, nullptr);
    }
}

void GraphicsPipelineV2::initCommon(GLFWwindow* window, VkRenderPass renderPass, VkShaderModule vsModule,
                                    VkShaderModule fsModule, int32_t numImages, VkFormat colorFormat,
                                    VkFormat depthFormat, VkCompareOp depthCompareOp)
{

    VkPipelineShaderStageCreateInfo shaderStagesCreateInfo[2]{
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vsModule,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fsModule,
            .pName = "main",
        }};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    int32_t windowWidth, windowHeight;
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

    VkViewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(windowWidth),
        .height = static_cast<float>(windowHeight),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor{
        .offset = {0, 0},
        .extent = {static_cast<uint32_t>(windowWidth), static_cast<uint32_t>(windowHeight)},
    };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = depthCompareOp,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = VK_FALSE,
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    VkPipelineRenderingCreateInfo renderingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorFormat,
        .depthAttachmentFormat = depthFormat,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &mDescriptorSetLayout,
    };

    if (vkCreatePipelineLayout(mDevice, &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout.");
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = renderPass ? NULL : &renderingCreateInfo,
        .stageCount = 2,
        .pStages = &shaderStagesCreateInfo[0],
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizerCreateInfo,
        .pMultisampleState = &multisamplingCreateInfo,
        .pDepthStencilState = &depthStencilCreateInfo,
        .pColorBlendState = &colorBlendingCreateInfo,
        .layout = mPipelineLayout,
        .renderPass = renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mGraphicsPipeline) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline.");
    }

    std::cout << "Graphics pipeline created successfully." << std::endl;
}

void GraphicsPipelineV2::createDescriptorPool(int32_t maxSets)
{
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(maxSets * 2)}, // VB + IB
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(maxSets)},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(maxSets)},
    };

    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = 0,
        .maxSets = static_cast<uint32_t>(maxSets),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    if (vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool.");
    }

    std::cout << "Descriptor pool created successfully." << std::endl;
}

void GraphicsPipelineV2::createDescriptorSetLayout(bool isVB, bool isIB, bool isUniform, bool isTex2D, bool isCubemap)
{
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

    if (isVB)
    {
        VkDescriptorSetLayoutBinding VertexShaderLayoutBinding_VB = {
            .binding = V2_BindingVB,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };

        layoutBindings.push_back(VertexShaderLayoutBinding_VB);
    }

    if (isIB)
    {
        VkDescriptorSetLayoutBinding VertexShaderLayoutBinding_IB = {
            .binding = V2_BindingIB,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };

        layoutBindings.push_back(VertexShaderLayoutBinding_IB);
    }

    if (isUniform)
    {
        VkDescriptorSetLayoutBinding VertexShaderLayoutBinding_Uniform = {
            .binding = V2_BindingUniform,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };

        layoutBindings.push_back(VertexShaderLayoutBinding_Uniform);
    }

    if (isTex2D)
    {
        VkDescriptorSetLayoutBinding FragmentShaderLayoutBinding_Tex = {
            .binding = V2_BindingTexture2D,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        };

        layoutBindings.push_back(FragmentShaderLayoutBinding_Tex);
    }

    if (isCubemap)
    {
        VkDescriptorSetLayoutBinding FragmentShaderLayoutBinding_TexCube = {
            .binding = V2_BindingTextureCube,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        };

        layoutBindings.push_back(FragmentShaderLayoutBinding_TexCube);
    }

    VkDescriptorSetLayoutCreateInfo LayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0, // reserved - must be zero
        .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
        .pBindings = layoutBindings.data(),
    };

    if (vkCreateDescriptorSetLayout(mDevice, &LayoutInfo, NULL, &mDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout.");
    }

    std::cout << "Descriptor set layout created successfully." << std::endl;
}

void GraphicsPipelineV2::allocateDescriptorSetsInternal(int32_t numSubmeshes,
                                                        std::vector<std::vector<VkDescriptorSet>>& descriptorSets)
{
    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mDescriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(numSubmeshes),
        .pSetLayouts = nullptr, // will be set later
    };

    descriptorSets.resize(mNumImages);
    for (int32_t imageIndex{0}; imageIndex < mNumImages; ++imageIndex)
    {
        descriptorSets[imageIndex].resize(numSubmeshes);
        std::vector<VkDescriptorSetLayout> layouts(numSubmeshes, mDescriptorSetLayout);
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(mDevice, &allocInfo, descriptorSets[imageIndex].data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate descriptor sets.");
        }
    }

    std::cout << "Descriptor sets allocated successfully." << std::endl;
}

} // namespace VulkanCore
