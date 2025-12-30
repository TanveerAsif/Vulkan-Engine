#include "VulkanModel.h"
#include "Material.h"
#include <cstddef>
#include <cstdint>

#include <iostream>
#include <stdexcept>
#include <vector>

#include <glm/ext/matrix_float4x4.hpp>
#include <vulkan/vulkan_core.h>

namespace VulkanCore
{
VulkanModel::VulkanModel(std::string modelPath, VulkanCore* pVulkanCore) : Model(), mVulkanCore(pVulkanCore)
{
    initScene(modelPath);
}

void VulkanModel::populateBuffer(std::vector<Vertex>& vertices)
{
    // Populate the vertex using PVP style
    mVertexSize = sizeof(Vertex);

    updateAlignedMeshesArray();
    createBuffers(vertices);
}

void VulkanModel::updateAlignedMeshesArray()
{
    mAlignedMeshes.resize(m_Meshes.size());
    VkDeviceSize alignment = mVulkanCore->getPhysicalDeviceLimits().minStorageBufferOffsetAlignment;

    size_t BaseVertexOffset{0};
    size_t BaseIndexOffset{0};
    for (size_t meshIndex = 0; meshIndex < m_Meshes.size(); meshIndex++)
    {
        // VB offset - align to storage buffer alignment
        mAlignedMeshes[meshIndex].VertexBufferOffset = BaseVertexOffset;
        mAlignedMeshes[meshIndex].VertexBufferRange = m_Meshes[meshIndex].NumVertices * mVertexSize;

        BaseVertexOffset += mAlignedMeshes[meshIndex].VertexBufferRange;
        BaseVertexOffset = (BaseVertexOffset + alignment - 1) & ~(alignment - 1); // align to next boundary

        // IB offset - align to storage buffer alignment
        mAlignedMeshes[meshIndex].IndexBufferOffset = BaseIndexOffset;
        mAlignedMeshes[meshIndex].IndexBufferRange = m_Meshes[meshIndex].NumIndices * sizeof(uint32_t);

        BaseIndexOffset += mAlignedMeshes[meshIndex].IndexBufferRange;
        BaseIndexOffset = (BaseIndexOffset + alignment - 1) & ~(alignment - 1); // align to next boundary
    }
}

void VulkanModel::createBuffers(std::vector<Vertex>& vertices)
{
    size_t numSubMeshes = m_Meshes.size();
    // std::cout << "Creating buffers for " << numSubMeshes << " submeshes" << std::endl;

    // Calculate total sizes : last buffer = offset + range
    size_t vertexBufferSize =
        mAlignedMeshes[numSubMeshes - 1].VertexBufferOffset + mAlignedMeshes[numSubMeshes - 1].VertexBufferRange;

    // Calculate total sizes : last buffer = offset + range
    size_t indexBufferSize =
        mAlignedMeshes[numSubMeshes - 1].IndexBufferOffset + mAlignedMeshes[numSubMeshes - 1].IndexBufferRange;

    char* pAlignedVertices = (char*)malloc(vertexBufferSize);
    char* pSrcVertices = (char*)vertices.data();

    char* pAlignedIndices = (char*)malloc(indexBufferSize);
    char* pSrcIndices = (char*)m_Indices.data();

    for (size_t meshIndex = 0; meshIndex < numSubMeshes; meshIndex++)
    {
        // std::cout << "Mesh " << meshIndex << ": " << m_Meshes[meshIndex].NumVertices << " verts, "
        //           << m_Meshes[meshIndex].NumIndices << " indices" << std::endl;

        // Copy vertices
        size_t srcOffset = m_Meshes[meshIndex].BaseVertex * mVertexSize;
        char* pSrc = pSrcVertices + srcOffset;
        char* pDst = pAlignedVertices + mAlignedMeshes[meshIndex].VertexBufferOffset;
        size_t copySize = mAlignedMeshes[meshIndex].VertexBufferRange;
        memcpy(pDst, pSrc, copySize);

        // Copy indices
        srcOffset = m_Meshes[meshIndex].BaseIndex * sizeof(uint32_t);
        pSrc = pSrcIndices + srcOffset;
        pDst = pAlignedIndices + mAlignedMeshes[meshIndex].IndexBufferOffset;
        copySize = mAlignedMeshes[meshIndex].IndexBufferRange;
        memcpy(pDst, pSrc, copySize);

        // // Debug: Print first few indices to verify they're mesh-relative
        // uint32_t* pIndices = (uint32_t*)(pSrcIndices + m_Meshes[meshIndex].BaseIndex * sizeof(uint32_t));
        // std::cout << "  First 3 indices: " << pIndices[0] << ", " << pIndices[1] << ", " << pIndices[2] << std::endl;
    }

    mVertexBuffer = mVulkanCore->createVertexBuffer(pAlignedVertices, vertexBufferSize);
    mIndexBuffer = mVulkanCore->createVertexBuffer(pAlignedIndices, indexBufferSize);
    mUniformBuffers = mVulkanCore->createUniformBuffers(sizeof(glm::mat4) * m_Meshes.size());

    free(pAlignedVertices);
    free(pAlignedIndices);
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
        if ((materialIndex >= 0) && (m_Materials[materialIndex].mpTextures[model::TEXTURE_TYPE::TEX_TYPE_BASE]))
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

        // VB offset - align to storage buffer alignment
        size_t offset = mAlignedMeshes[meshIndex].VertexBufferOffset;
        size_t range = mAlignedMeshes[meshIndex].VertexBufferRange;

        desc.mRanges[meshIndex].mVbRange = {.mOffset = offset, .mRange = range};

        offset = mAlignedMeshes[meshIndex].IndexBufferOffset;
        range = mAlignedMeshes[meshIndex].IndexBufferRange;
        desc.mRanges[meshIndex].mIbRange = {
            .mOffset = offset,
            .mRange = range,
        };

        offset = meshIndex * sizeof(glm::mat4);
        range = sizeof(glm::mat4);
        desc.mRanges[meshIndex].mUniformRange = {.mOffset = offset, .mRange = range};
    }
}

void VulkanModel::recordCommandBuffer(VkCommandBuffer commandBuffer, GraphicsPipelineV2* pPipeline, uint32_t imageIndex)
{
    uint32_t instanceCount{1};

    uint32_t numSubmeshes = static_cast<uint32_t>(m_Meshes.size());
    for (uint32_t submeshIndex = 0; submeshIndex < numSubmeshes; submeshIndex++)
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->getPipelineLayout(),
                                0, // firstSet
                                1, // descriptorSetCount
                                &mDescriptorSets[imageIndex][submeshIndex],
                                0,        // dynamicOffsetCount
                                nullptr); // pDynamicOffsets

        // Draw using index count with manual index buffer reading in shader
        uint32_t indexCount = m_Meshes[submeshIndex].NumIndices;
        uint32_t firstIndex = 0; // Always 0 since we bind with offset in descriptor
        uint32_t firstInstance = 0;
        vkCmdDraw(commandBuffer, indexCount, instanceCount, firstIndex, firstInstance);
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
