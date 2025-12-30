#pragma once
#include <vulkan/vulkan_core.h>

#include "Core.h"
#include "Texture.h"
#include <cstddef>

namespace VulkanCore
{

struct SimpleMesh
{
    BufferAndMemory mVertexBuffer{};
    size_t mVertexBufferSize = 0;
    Texture* mTexture = nullptr;

    void Destroyed(VkDevice device)
    {
        mVertexBuffer.Destroy(device);
        if (mTexture)
        {
            mTexture->destroy(device);
            delete mTexture;
            mTexture = nullptr;
        }
    }
};

} // namespace VulkanCore
