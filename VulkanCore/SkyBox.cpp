#include "SkyBox.h"
#include "Shader.h"
#include "Wrapper.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace VulkanCore
{
SkyBox::SkyBox(VulkanCore* vulkanCore, std::string fileName)
    : mVulkanCore{vulkanCore}, mNumImages{0}, mCubemapTexture{new Texture(vulkanCore)}, mUniformBuffers{},
      mDescriptorSets{}, mVertexShaderModule{VK_NULL_HANDLE}, mFragmentShaderModule{VK_NULL_HANDLE}, mGraphicsPipeline{
                                                                                                         nullptr}
{
    mNumImages = mVulkanCore->getSwapchainImageCount();
    mUniformBuffers = mVulkanCore->createUniformBuffers(sizeof(glm::mat4));

    mCubemapTexture->loadEctCubemap(fileName);

    mVertexShaderModule = CreateShaderModuleFromText(mVulkanCore->getDevice(), "VulkanCore/shaders/skybox.vert");
    mFragmentShaderModule = CreateShaderModuleFromText(mVulkanCore->getDevice(), "VulkanCore/shaders/skybox.frag");

    PipelineDesc pd;
    pd.mDevice = mVulkanCore->getDevice();
    pd.mWindow = mVulkanCore->getWindow();
    pd.mVertexShaderModule = mVertexShaderModule;
    pd.mFragmentShaderModule = mFragmentShaderModule;
    pd.mNumSwapchainImages = mNumImages;
    pd.mColorFormat = mVulkanCore->getSwapchainSurfaceFormat();
    pd.mDepthFormat = mVulkanCore->getDepthFormat();
    pd.mDepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // important for skybox
    pd.mCullMode = VK_CULL_MODE_FRONT_BIT;            // Cull front faces since we're inside the cube
    pd.mIsUniform = true;
    pd.mIsCubemap = true;

    mGraphicsPipeline = new GraphicsPipelineV2(pd);
    createDescriptorSets();
}

void SkyBox::createDescriptorSets()
{
    int32_t numSubMeshes{1};
    mGraphicsPipeline->allocateDescriptorSets(numSubMeshes, mDescriptorSets);

    int32_t numBindings{2}; // binding 0 : uniform buffer, binding 1 : cubemap sampler
    std::vector<VkWriteDescriptorSet> writeDescriptorSets(mNumImages * numBindings);

    VkDescriptorImageInfo imageInfo = {
        .sampler = mCubemapTexture->mSampler,
        .imageView = mCubemapTexture->mImageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    std::vector<VkDescriptorBufferInfo> bufferInfo_Uniform(mNumImages);

    int32_t WdsIndex = 0;
    for (int32_t ImageIndex = 0; ImageIndex < mNumImages; ImageIndex++)
    {
        bufferInfo_Uniform[ImageIndex].buffer = mUniformBuffers[ImageIndex].mBuffer;
        bufferInfo_Uniform[ImageIndex].offset = 0;
        bufferInfo_Uniform[ImageIndex].range = VK_WHOLE_SIZE;
    }

    for (int32_t ImageIndex = 0; ImageIndex < mNumImages; ImageIndex++)
    {
        VkDescriptorSet DstSet = mDescriptorSets[ImageIndex][0];

        VkWriteDescriptorSet wds = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                    .dstSet = DstSet,
                                    .dstBinding = V2_BindingUniform,
                                    .dstArrayElement = 0,
                                    .descriptorCount = 1,
                                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                    .pBufferInfo = &bufferInfo_Uniform[ImageIndex]};

        assert(WdsIndex < static_cast<int32_t>(writeDescriptorSets.size()));
        writeDescriptorSets[WdsIndex++] = wds;
        wds = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
               .dstSet = DstSet,
               .dstBinding = V2_BindingTextureCube,
               .dstArrayElement = 0,
               .descriptorCount = 1,
               .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
               .pImageInfo = &imageInfo};

        assert(WdsIndex < static_cast<int32_t>(writeDescriptorSets.size()));
        writeDescriptorSets[WdsIndex++] = wds;
    }

    vkUpdateDescriptorSets(mVulkanCore->getDevice(), (uint32_t)writeDescriptorSets.size(), writeDescriptorSets.data(),
                           0, nullptr);
}

SkyBox::~SkyBox()
{
    destroy();
}

void SkyBox::destroy()
{
    if (mGraphicsPipeline)
    {
        delete mGraphicsPipeline;
        mGraphicsPipeline = nullptr;
    }

    if (mVertexShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(mVulkanCore->getDevice(), mVertexShaderModule, nullptr);
        mVertexShaderModule = VK_NULL_HANDLE;
    }

    if (mFragmentShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(mVulkanCore->getDevice(), mFragmentShaderModule, nullptr);
        mFragmentShaderModule = VK_NULL_HANDLE;
    }

    for (auto& uniformBuffer : mUniformBuffers)
    {
        uniformBuffer.Destroy(mVulkanCore->getDevice());
    }

    mCubemapTexture->destroy(mVulkanCore->getDevice());
}

void SkyBox::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    mGraphicsPipeline->bind(commandBuffer);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline->getPipelineLayout(), 0,
                            1, &mDescriptorSets[imageIndex][0], 0, nullptr);

    int32_t baseVertex{0};
    int32_t firstInstance{0};
    int32_t instanceCount{1};
    int32_t numVertices{36}; // 36 vertices for a cube

    vkCmdDraw(commandBuffer, numVertices, instanceCount, baseVertex, firstInstance);
}

void SkyBox::update(int32_t imageIndex, const glm::mat4& transformation)
{
    mUniformBuffers[imageIndex].update(mVulkanCore->getDevice(), glm::value_ptr(transformation),
                                       sizeof(transformation));
}

} // namespace VulkanCore
