#include "Mesh.h"

namespace VulkanCore::model
{
void MeshData::destroy(VkDevice device)
{
    mVertexBuffer.Destroy(device);
    mIndexBuffer.Destroy(device);
}
} // namespace VulkanCore::model
