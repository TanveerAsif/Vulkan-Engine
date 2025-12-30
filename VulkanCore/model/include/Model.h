#ifndef MODEL_H
#define MODEL_H

#include <assimp/material.h>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "Material.h"

namespace VulkanCore::model
{
struct BasicMeshEntry
{
    uint32_t BaseVertex{0};
    uint32_t BaseIndex{0};
    uint32_t NumVertices{0};
    uint32_t NumIndices{0};
    uint32_t ValidFaces{0};
    int32_t MaterialIndex{-1};
    glm::mat4 Transformation;
};

class Model
{
  public:
    Model(std::string modelPath);
    virtual ~Model();

  protected:
    virtual Texture* allocTexture2D() = 0;
    virtual void destroyTexture(Texture* pTexture) = 0;

    // Helper method for derived classes to call in their destructors
    void destroyAllTextures()
    {
        for (CoreMaterial& material : m_Materials)
        {
            for (Texture* texture : material.mpTextures)
            {
                if (texture)
                {
                    destroyTexture(texture);
                    texture = nullptr;
                }
            }
        }
    }

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec2 texCoord;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 bitangent;
        glm::vec4 color;
    };

    struct SkinnedVertex
    {
        // ToDo: implement skinned vertex
    };

    std::vector<BasicMeshEntry> m_Meshes;
    std::vector<CoreMaterial> m_Materials;
    std::vector<uint32_t> m_Indices;

  private:
    template <typename VertexType>
    void initGeometryInternal(std::vector<VertexType>& vertices, uint32_t NumVertices, uint32_t NumIndices,
                              const aiScene* pScene)
    {
        reserveSpace<VertexType>(vertices, NumVertices, NumIndices);
        initAllMeshes<VertexType>(vertices, pScene);
    }

    template <typename VertexType>
    void reserveSpace(std::vector<VertexType>& vertices, uint32_t NumVertices, uint32_t NumIndices)
    {
        vertices.resize(NumVertices);
        m_Indices.resize(NumIndices);
    }

    template <typename VertexType> void initAllMeshes(std::vector<VertexType>& vertices, const aiScene* pScene)
    {
        const bool useMeshOptimizer{false}; // ToDo: implement mesh optimizer
        for (uint32_t i = 0; i < m_Meshes.size(); i++)
        {
            const aiMesh* mesh = pScene->mMeshes[i];
            if (useMeshOptimizer)
            {
                // ToDo : implement mesh optimizer library
                // remove duplicate vertices and reindex
                // cache locality optimization
                // reduce overdraw
                // vertex fetch optimization
                // create simplified version of mesh for LOD
            }
            else
            {
                initSingleMesh<VertexType>(vertices, mesh, i);
            }
        }
    }

    template <typename VertexType>
    void initSingleMesh(std::vector<VertexType>& vertices, const aiMesh* mesh, uint32_t meshIndex)
    {
        VertexType vertex;

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {

            // Process vertex positions
            vertex.pos = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            // Process normals
            if (mesh->HasNormals())
            {
                vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            }
            else
            {
                vertex.normal = glm::vec3(0.0f, 0.0f, 0.0f);
            }

            // Process texture coordinates
            if (mesh->HasTextureCoords(0))
            {
                vertex.texCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            }
            else
            {
                vertex.texCoord = glm::vec2(0.0f, 0.0f);
            }

            // Process tangents and bitangents
            if (mesh->HasTangentsAndBitangents())
            {
                vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                vertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
            }
            else
            {
                vertex.tangent = glm::vec3(0.0f, 0.0f, 0.0f);
                vertex.bitangent = glm::vec3(0.0f, 0.0f, 0.0f);
            }

            vertices[i] = vertex;
        }

        // Process indices
        for (uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
            {
                m_Indices.push_back(face.mIndices[j]);
            }
        }

        // ToDo : handle bones for skinned meshes
    }

    bool initGeometry(const aiScene* pScene, const std::string& Filename);
    bool initMaterials(const aiScene* pScene, const std::string& Filename);

    void countVerticesAndIndices(const aiScene* pScene, uint32_t& NumVertices, uint32_t& NumIndices);
    void loadTexture(const std::string& dir, const aiMaterial* pMaterial, int32_t materialIndex,
                     TEXTURE_TYPE myTexType);
    void loadTexturesFromMaterial(const aiMaterial* pMaterial, const std::string& Filename, int32_t materialIndex);
    void loadTextureFromFile(const std::string& Dir, const std::string& Path, int32_t MaterialIndex,
                             TEXTURE_TYPE MyType, bool IsSRGB);

    void loadDiffuseTexture(const std::string& dir, const aiMaterial* pMaterial, int32_t materialIndex);
    void loadSpecularTexture(const std::string& dir, const aiMaterial* pMaterial, int32_t materialIndex);
    void loadNormalTexture(const std::string& dir, const aiMaterial* pMaterial, int32_t materialIndex);

    void loadColorFromMaterial(const aiMaterial* pMaterial, int32_t materialIndex);
    void loadColor(const aiMaterial* pMaterial, glm::vec4& color, const char* pAiMatKey, int32_t aiMatType,
                   int32_t AiMatIdx);

    void calculateMeshTransformations(const aiScene* pScene);
    void traverseNodeHierarchy(const glm::mat4& ParentTransformation, aiNode* pNode);

    virtual void populateBufferSkinned(std::vector<Vertex>& vertices) = 0;
    virtual void populateBuffer(std::vector<Vertex>& vertices) = 0;

    const aiScene* m_pScene;
};
} // namespace VulkanCore::model

#endif // MODEL_H
