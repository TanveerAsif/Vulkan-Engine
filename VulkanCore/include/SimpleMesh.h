#pragma once
#include <vulkan/vulkan_core.h>

#include "Core.h"
#include <cstddef>

namespace VulkanCore {

struct SimpleMesh {
  BufferAndMemory mVertexBuffer{};
  size_t mVertexBufferSize = 0;

  void Destroyed(VkDevice device) { mVertexBuffer.Destroy(device); }
};

} // namespace VulkanCore
