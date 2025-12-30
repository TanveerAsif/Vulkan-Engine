#ifndef MODEL_DESC_H
#define MODEL_DESC_H

#include <vector>
#include <vulkan/vulkan_core.h>
namespace VulkanCore
{
struct RangeDesc
{
    VkDeviceSize mOffset = 0;
    VkDeviceSize mRange = 0;
};

struct SubmeshRanges
{
    RangeDesc mVbRange;
    RangeDesc mIbRange;
    RangeDesc mUniformRange;
};

struct TextureInfo
{
    VkSampler mSampler;
    VkImageView mImageView;
};

struct ModelDesc
{
    VkBuffer mVertexBuffer;
    VkBuffer mIndexBuffer;
    std::vector<VkBuffer> mUniformBuffers;
    std::vector<TextureInfo> mMaterials;
    std::vector<SubmeshRanges> mRanges;
};
} // namespace VulkanCore

#endif // MODEL_DESC_H
