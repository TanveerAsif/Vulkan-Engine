#pragma once
#include <vulkan/vulkan_core.h>

#include "Core.h"
#include <cstddef>

namespace VulkanCore {

struct SimpleMesh {
  BufferAndMemory mVertexBuffer{};
  size_t mVertexBufferSize = 0;
  VulkanTexture *mTexture = nullptr;

  void Destroyed(VkDevice device) {
    mVertexBuffer.Destroy(device);
    if (mTexture) {
      mTexture->Destroy(device);
      delete mTexture;
      mTexture = nullptr;
    }
  }
};

} // namespace VulkanCore
