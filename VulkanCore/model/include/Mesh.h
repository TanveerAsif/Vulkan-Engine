#ifndef MODEL_MESH_H
#define MODEL_MESH_H

#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

#include <cstdint>

#include <string>
#include <vector>

#include "Core.h"

namespace VulkanCore::model
{
struct Mesh
{
    uint32_t mBaseVertex{0};
    uint32_t mBaseIndex{0};
    uint32_t mNumIndices{0};
    uint32_t mNumVertices{0};
    uint32_t mMaterialId{0};
};

struct MeshMaterial
{
    glm::vec4 m_emissiveFactor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 m_baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float m_roughness = 1.0f;
    float m_transparencyFactor = 1.0f;
    float m_alphaTest = 0.0f;
    float m_metallicFactor = 0.0f;
    // index into MeshData::textureFiles
    int m_baseColorTexture = -1;
    int m_emissiveTexture = -1;
    int m_normalTexture = -1;
    int m_opacityTexture = -1;
};

class MeshData
{
  public:
    std::vector<Mesh> m_meshes;
    std::vector<MeshMaterial> m_materials;
    std::vector<std::string> textureFiles;
    uint32_t m_totalNumIndices{0};
    uint32_t m_totalNumVertices{0};
    uint32_t mVertexSize{0};

    BufferAndMemory mVertexBuffer;
    BufferAndMemory mIndexBuffer;

    void destroy(VkDevice device);
};

} // namespace VulkanCore::model

#endif // MODEL_MESH_H
