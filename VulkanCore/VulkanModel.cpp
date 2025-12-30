#include "VulkanModel.h"
#include "Material.h"
#include <cstdint>

#include <iostream>
#include <stdexcept>
#include <vector>

#include <glm/ext/matrix_float4x4.hpp>
#include <vulkan/vulkan_core.h>

namespace VulkanCore
{
VulkanModel::VulkanModel(std::string modelPath, VulkanCore* pVulkanCore) : Model(modelPath), mVulkanCore(pVulkanCore) {}

void VulkanModel::populateBuffer(std::vector<Vertex>& vertices)
{
    // Populate the vertex using PVP style
    mVertexBuffer = mVulkanCore->createVertexBuffer(vertices.data(), sizeof(Vertex) * vertices.size());
    mIndexBuffer = mVulkanCore->createVertexBuffer(m_Indices.data(), sizeof(uint32_t) * m_Indices.size());
    mUniformBuffers = mVulkanCore->createUniformBuffers(sizeof(glm::mat4) * m_Meshes.size());
    mVertexSize = sizeof(Vertex);
}

Texture* VulkanModel::allocTexture2D()
{
    assert(mVulkanCore != nullptr);
    return new Texture(mVulkanCore);
}

void VulkanModel::destroyTexture(Texture* pTexture)
{
    if (pTexture)
    {
        pTexture->destroy(mVulkanCore->getDevice());
        delete pTexture;
        pTexture = nullptr;
    }
}

void VulkanModel::destroy()
{
    mVertexBuffer.Destroy(mVulkanCore->getDevice());
    mIndexBuffer.Destroy(mVulkanCore->getDevice());

    for (auto& uniformBuffer : mUniformBuffers)
    {
        uniformBuffer.Destroy(mVulkanCore->getDevice());
    }

    // Destroy material textures
    destroyAllTextures();
}

void VulkanModel::createDescriptorSets(GraphicsPipelineV2* pPipeline)
{
    int32_t numSubmeshes = static_cast<int32_t>(m_Meshes.size());
    pPipeline->allocateDescriptorSets(numSubmeshes, mDescriptorSets);
    ModelDesc modelDesc;
    updateModelDesc(modelDesc);
    pPipeline->updateDescriptorSets(modelDesc, mDescriptorSets);
}

void VulkanModel::updateModelDesc(ModelDesc& desc)
{
    desc.mVertexBuffer = mVertexBuffer.mBuffer;
    desc.mIndexBuffer = mIndexBuffer.mBuffer;
    desc.mUniformBuffers.resize(mVulkanCore->getSwapchainImageCount());
    for (size_t imageIndex = 0; imageIndex < mUniformBuffers.size(); imageIndex++)
    {
        desc.mUniformBuffers[imageIndex] = mUniformBuffers[imageIndex].mBuffer;
    }

    desc.mRanges.resize(m_Meshes.size());
    desc.mMaterials.resize(m_Meshes.size());

    int32_t numSubmeshes = static_cast<int32_t>(m_Meshes.size());
    for (int32_t meshIndex = 0; meshIndex < numSubmeshes; meshIndex++)
    {
        int32_t materialIndex = m_Meshes[meshIndex].MaterialIndex;
        if ((materialIndex >= 0) && (materialIndex < static_cast<int32_t>(m_Materials.size())))
        {
            Texture* pDiffuse = m_Materials[materialIndex].mpTextures[model::TEXTURE_TYPE::TEX_TYPE_BASE];
            desc.mMaterials[meshIndex].mImageView = pDiffuse->mImageView;
            desc.mMaterials[meshIndex].mSampler = pDiffuse->mSampler;
        }
        else
        {
            desc.mMaterials[meshIndex].mImageView = VK_NULL_HANDLE;
            desc.mMaterials[meshIndex].mSampler = VK_NULL_HANDLE;

            std::cout << "no diffuse texture for mesh index " << meshIndex << std::endl;
            throw std::runtime_error("Invalid material index in VulkanModel::updateModelDesc");
        }

        size_t offset = m_Meshes[meshIndex].BaseVertex * mVertexSize;
        size_t range = m_Meshes[meshIndex].NumVertices * mVertexSize;
        desc.mRanges[meshIndex].mVbRange = RangeDesc{.mOffset = offset, .mRange = range};

        offset = m_Meshes[meshIndex].BaseIndex * sizeof(uint32_t);
        range = m_Meshes[meshIndex].NumIndices * sizeof(uint32_t);
        desc.mRanges[meshIndex].mIbRange = RangeDesc{.mOffset = offset, .mRange = range};

        offset = meshIndex * sizeof(glm::mat4);
        range = sizeof(glm::mat4);
        desc.mRanges[meshIndex].mUniformRange = RangeDesc{.mOffset = offset, .mRange = range};
    }
}

void VulkanModel::recordCommandBuffer(VkCommandBuffer commandBuffer, GraphicsPipelineV2* pPipeline, uint32_t imageIndex)
{
    uint32_t instanceCount{1};
    uint32_t baseVertex{0};

    uint32_t numSubmeshes = static_cast<uint32_t>(m_Meshes.size());
    for (uint32_t submeshIndex = 0; submeshIndex < numSubmeshes; submeshIndex++)
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->getPipelineLayout(),
                                0, // firstSet
                                1, // descriptorSetCount
                                &mDescriptorSets[imageIndex][submeshIndex],
                                0,        // dynamicOffsetCount
                                nullptr); // pDynamicOffsets

        vkCmdDraw(commandBuffer, m_Meshes[submeshIndex].NumIndices, instanceCount, baseVertex, submeshIndex);
    }
}

void VulkanModel::update(int currentImage, const glm::mat4 transformation)
{
    std::vector<glm::mat4> transformations(m_Meshes.size());
    for (size_t meshIndex = 0; meshIndex < m_Meshes.size(); meshIndex++)
    {
        glm::mat4 meshTransform = m_Meshes[meshIndex].Transformation;
        transformations[meshIndex] = transformation * meshTransform;
    }

    mUniformBuffers[currentImage].update(mVulkanCore->getDevice(), transformations.data(),
                                         sizeof(glm::mat4) * transformations.size());
}

} // namespace VulkanCore
