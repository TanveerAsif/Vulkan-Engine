#ifndef VULKANCORE_MODEL_H
#define VULKANCORE_MODEL_H

#include "Core.h"
#include "GraphicsPipelineV2.h"
#include "Model.h"
#include "Texture.h"

#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace VulkanCore
{
class VulkanModel : public model::Model
{
  public:
    VulkanModel(std::string modelPath, VulkanCore* pVulkanCore);
    ~VulkanModel() = default;

    void destroy();
    void createDescriptorSets(GraphicsPipelineV2* pPipeline);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, GraphicsPipelineV2* pPipeline, uint32_t imageIndex);
    void update(int currentImage, const glm::mat4 transformation);

    const BufferAndMemory& getVertexBuffer() const
    {
        return mVertexBuffer;
    }
    const BufferAndMemory& getIndexBuffer() const
    {
        return mIndexBuffer;
    }
    uint32_t getVertexSize() const
    {
        return mVertexSize;
    }

  protected:
    Texture* allocTexture2D() override;
    void destroyTexture(Texture* pTexture) override;
    void populateBuffer(std::vector<Vertex>& vertices) override;
    void populateBufferSkinned(std::vector<Vertex>& vertices) override
    {
        throw std::runtime_error("Skinned meshes not supported yet.");
    };

  private:
    void updateModelDesc(ModelDesc& desc);

    VulkanCore* mVulkanCore;

    BufferAndMemory mVertexBuffer;
    BufferAndMemory mIndexBuffer;
    std::vector<BufferAndMemory> mUniformBuffers;
    std::vector<std::vector<VkDescriptorSet>> mDescriptorSets;
    uint32_t mVertexSize{0}; // sizeof(Vertex) or sizeof(SkinnedVertex)
};

} // namespace VulkanCore

#endif // VULKANCORE_MODEL_H
