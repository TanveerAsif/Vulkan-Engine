#include "Model.h"
#include "include/Material.h"
#include <assimp/material.h>
#include <cassert>
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <iostream>
#include <vector>

namespace VulkanCore::model
{

namespace
{

aiTextureType getAssimpTextureType(TEXTURE_TYPE texType)
{
    switch (texType)
    {
        case TEX_TYPE_BASE:
            return aiTextureType_DIFFUSE;
        case TEX_TYPE_SPECULAR:
            return aiTextureType_SPECULAR;
        case TEX_TYPE_NORMAL:
            return aiTextureType_NORMALS;
        default:
            return aiTextureType_UNKNOWN;
    }
}

glm::mat4 convertGLMmatrix4(const aiMatrix4x4& aiMat)
{
    glm::mat4 mat;
    mat[0][0] = aiMat.a1;
    mat[1][0] = aiMat.a2;
    mat[2][0] = aiMat.a3;
    mat[3][0] = aiMat.a4;
    mat[0][1] = aiMat.b1;
    mat[1][1] = aiMat.b2;
    mat[2][1] = aiMat.b3;
    mat[3][1] = aiMat.b4;
    mat[0][2] = aiMat.c1;
    mat[1][2] = aiMat.c2;
    mat[2][2] = aiMat.c3;
    mat[3][2] = aiMat.c4;
    mat[0][3] = aiMat.d1;
    mat[1][3] = aiMat.d2;
    mat[2][3] = aiMat.d3;
    mat[3][3] = aiMat.d4;
    return mat;
}

void setMaterialType(const aiMaterial* pMaterial, CoreMaterial& MyMaterial)
{
    int ShadingModel = 0;
    if (pMaterial->Get(AI_MATKEY_SHADING_MODEL, ShadingModel) != AI_SUCCESS)
    {
        std::cout << "Cannot get the shading model\n";
        assert(0);
    }

    model::MaterialType mt{model::MaterialType::MaterialType_Invalid};

    switch (ShadingModel)
    {
        case aiShadingMode_Unlit:
            std::cout << "Shading model: Unlit\n";
            mt = model::MaterialType::MaterialType_Unlit;
            break;

        case aiShadingMode_PBR_BRDF:
        {
            std::cout << "Shading model: PBR BRDF\n";
            MyMaterial.m_isPBR = true;
            float factor = 0;
            if (pMaterial->Get(AI_MATKEY_METALLIC_FACTOR, factor) == AI_SUCCESS)
            {
                mt = model::MaterialType::MaterialType_MetallicRoughness;
            }
        }
        break;

        case aiShadingMode_Gouraud:
            std::cout << "Shading model: Gouraud\n";
            break;

        case aiShadingMode_Phong:
            std::cout << "Shading model: Phong\n";
            break;

        default:
            assert(0);
    }

    MyMaterial.m_materialType = mt;
}

int32_t countValidFaces(const aiMesh* mesh)
{
    int32_t validFaces = 0;
    for (uint32_t i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        if (face.mNumIndices == 3) // only count triangles
        {
            validFaces++;
        }
    }
    return validFaces;
}

int32_t getTextureCount(const aiMaterial* pMaterial)
{
    int32_t textureCount{0};
    for (int32_t i{0}; i <= AI_TEXTURE_TYPE_MAX; ++i)
    {
        aiTextureType texType = static_cast<aiTextureType>(i);
        textureCount += pMaterial->GetTextureCount(texType);
    }
    return textureCount;
}

bool getFullTransformation(const aiNode* pRootNode, const char* pName, glm::mat4& Transformation)
{
    Transformation = glm::mat4(1.0f);

    const aiNode* pNode = pRootNode->FindNode(pName);
    if (!pNode)
    {
        printf("Warning! Cannot find a node for '%s'\n", pName);
        return false;
    }

    while (pNode)
    {
        glm::mat4 CurTransformation = convertGLMmatrix4(pNode->mTransformation);
        Transformation = Transformation * CurTransformation;
        pNode = pNode->mParent;
    }

    return true;
}

} // namespace

bool Model::initGeometry(const aiScene* pScene, const std::string& Filename)
{
    m_Meshes.resize(pScene->mNumMeshes);
    m_Materials.resize(pScene->mNumMaterials);

    uint32_t NumVertices{0};
    uint32_t NumIndices{0};

    countVerticesAndIndices(pScene, NumVertices, NumIndices);

    // initialize buffers
    if (pScene->mNumAnimations > 0)
    {
        // ToDo : Handle skinned meshes
        throw std::runtime_error("Skinned meshes not supported yet.");
    }
    else
    {
        std::vector<Vertex> Vertices;
        initGeometryInternal<Vertex>(Vertices, NumVertices, NumIndices, pScene);
        populateBuffer(Vertices);
    }

    if (!initMaterials(pScene, Filename))
    {
        return false;
    }

    calculateMeshTransformations(pScene);

    return true;
}

void Model::countVerticesAndIndices(const aiScene* pScene, uint32_t& NumVertices, uint32_t& NumIndices)
{
    for (uint32_t i = 0; i < m_Meshes.size(); i++)
    {
        m_Meshes[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;
        m_Meshes[i].ValidFaces = countValidFaces(pScene->mMeshes[i]);
        m_Meshes[i].NumVertices = pScene->mMeshes[i]->mNumVertices;
        m_Meshes[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3; // assuming all faces are triangles
        m_Meshes[i].BaseVertex = NumVertices;
        m_Meshes[i].BaseIndex = NumIndices;

        NumVertices += m_Meshes[i].NumVertices;
        NumIndices += m_Meshes[i].NumIndices;
    }
}

bool Model::initMaterials(const aiScene* pScene, const std::string& Filename)
{
    std::string dir;
    size_t slashIndex = Filename.find_last_of("/\\");
    if (slashIndex != std::string::npos)
    {
        dir = Filename.substr(0, slashIndex);
    }
    // All material data are in "textures" folder
    dir += "/textures";

    for (uint32_t i = 0; i < m_Materials.size(); i++)
    {
        const aiMaterial* pMaterial = pScene->mMaterials[i];
        setMaterialType(pMaterial, m_Materials[i]);
        loadTexturesFromMaterial(pMaterial, dir, i);
        loadColorFromMaterial(pMaterial, i);
    }

    return true;
}

void Model::loadTexturesFromMaterial(const aiMaterial* pMaterial, const std::string& dir, int32_t materialIndex)
{
    int32_t textureCount = getTextureCount(pMaterial);
    std::cout << "Material index " << materialIndex << " has " << textureCount << " textures." << std::endl;
    loadDiffuseTexture(dir, pMaterial, materialIndex);
    loadSpecularTexture(dir, pMaterial, materialIndex);
    loadNormalTexture(dir, pMaterial, materialIndex);

    /* ToDo: implement other texture types
    LoadMetalnessTexture(dir, pMaterial, materialIndex);
    LoadEmissiveTexture(dir, pMaterial, materialIndex);
    LoadEmissionColorTexture(dir, pMaterial, materialIndex);
    LoadNormalCameraTexture(dir, pMaterial, materialIndex);
    LoadRoughnessTexture(dir, pMaterial, materialIndex);
    LoadAmbientOcclusionTexture(dir, pMaterial, materialIndex);
    LoadClearCoatTexture(dir, pMaterial, materialIndex);
    LoadClearCoatRoughnessTexture(dir, pMaterial, materialIndex);
    LoadClearCoatNormalTexture(dir, pMaterial, materialIndex);
    */
}

void Model::loadTexture(const std::string& dir, const aiMaterial* pMaterial, int32_t materialIndex,
                        TEXTURE_TYPE texType)
{
    aiTextureType aiTexType = getAssimpTextureType(texType);
    m_Materials[materialIndex].mpTextures[texType] = nullptr;
    if (pMaterial->GetTextureCount(aiTexType) > 0)
    {
        aiString texturePath;
        if (pMaterial->GetTexture(aiTexType, 0, &texturePath, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
        {
            const aiTexture* paiTexture = m_pScene->GetEmbeddedTexture(texturePath.C_Str());
            bool isSRGB = false; //(texType == TEXTURE_TYPE::TEX_TYPE_BASE); //
                                 // assuming base color texture is sRGB

            // Clean up path by removing .\ or ./ prefix
            std::string cleanPath = texturePath.C_Str();
            if (cleanPath.size() >= 2 && cleanPath[0] == '.')
            {
                if (cleanPath[1] == '\\' || cleanPath[1] == '/')
                {
                    cleanPath = cleanPath.substr(2);
                }
            }

            if (paiTexture)
            {
                // ToDo : implement embedded texture loading
                // loadTextureEmbedded();
            }
            else
            {
                loadTextureFromFile(dir, cleanPath, materialIndex, texType, isSRGB);
            }
        }
    }
}

void Model::loadTextureFromFile(const std::string& Dir, const std::string& Path, int32_t MaterialIndex,
                                TEXTURE_TYPE MyType, bool IsSRGB)
{
    std::string fullPath = Dir + "/" + Path;
    m_Materials[MaterialIndex].mpTextures[MyType] = allocTexture2D();
    m_Materials[MaterialIndex].mpTextures[MyType]->LoadFromFile(fullPath);
    std::cout << "Loaded texture: " << fullPath << std::endl;
}

void Model::loadDiffuseTexture(const std::string& dir, const aiMaterial* pMaterial, int32_t materialIndex)
{
    loadTexture(dir, pMaterial, materialIndex, TEXTURE_TYPE::TEX_TYPE_BASE);
}

void Model::loadSpecularTexture(const std::string& dir, const aiMaterial* pMaterial, int32_t materialIndex)
{
    loadTexture(dir, pMaterial, materialIndex, TEXTURE_TYPE::TEX_TYPE_SPECULAR);
}

void Model::loadNormalTexture(const std::string& dir, const aiMaterial* pMaterial, int32_t materialIndex)
{
    loadTexture(dir, pMaterial, materialIndex, TEXTURE_TYPE::TEX_TYPE_NORMAL);
}

void Model::loadColorFromMaterial(const aiMaterial* pMaterial, int32_t materialIndex)
{
    CoreMaterial& material = m_Materials[materialIndex];
    material.m_name = pMaterial->GetName().C_Str();

    loadColor(pMaterial, material.mDiffuseColor, AI_MATKEY_COLOR_DIFFUSE);
    loadColor(pMaterial, material.mSpecularColor, AI_MATKEY_COLOR_SPECULAR);
    loadColor(pMaterial, material.mAmbientColor, AI_MATKEY_COLOR_AMBIENT);
    loadColor(pMaterial, material.mEmissiveColor, AI_MATKEY_COLOR_EMISSIVE);
    loadColor(pMaterial, material.mReflectiveColor, AI_MATKEY_COLOR_REFLECTIVE);
}

void Model::loadColor(const aiMaterial* pMaterial, glm::vec4& color, const char* pAiMatKey, int32_t aiMatType,
                      int32_t AiMatIdx)
{
    aiColor4D aiColor;
    if (pMaterial->Get(pAiMatKey, aiMatType, AiMatIdx, aiColor) == AI_SUCCESS)
    {
        color = glm::vec4(aiColor.r, aiColor.g, aiColor.b, aiColor.a);
    }
    else
    {
        color = glm::vec4(0.0f);
    }
}

void Model::calculateMeshTransformations(const aiScene* pScene)
{
    aiNode* rootNode = pScene->mRootNode;
    glm::mat4 identity = glm::mat4(1.0f);
    traverseNodeHierarchy(identity, rootNode);
}

void Model::traverseNodeHierarchy(const glm::mat4& ParentTransformation, aiNode* pNode)
{
    glm::mat4 nodeTransformation = convertGLMmatrix4(pNode->mTransformation);
    glm::mat4 combinedTransformation = ParentTransformation * nodeTransformation;

    for (uint32_t i = 0; i < pNode->mNumMeshes; i++)
    {
        uint32_t meshIndex = pNode->mMeshes[i];
        m_Meshes[meshIndex].Transformation = combinedTransformation;
    }

    for (uint32_t i = 0; i < pNode->mNumChildren; i++)
    {
        traverseNodeHierarchy(combinedTransformation, pNode->mChildren[i]);
    }
}

Model::Model() : m_pScene{nullptr} {}

Model::~Model()
{
    // Note: Texture destruction must be handled by derived class destructor
    // before this base destructor runs, since destroyTexture() is pure virtual.
    // The derived class should call destroyTextures() in its destructor.
}

void Model::initScene(std::string modelPath)
{
    // Load model from the specified path
    Assimp::Importer importer;
    m_pScene = importer.ReadFile(
        modelPath, aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                       aiProcess_LimitBoneWeights | aiProcess_SplitLargeMeshes | aiProcess_ImproveCacheLocality |
                       aiProcess_RemoveRedundantMaterials | aiProcess_FindDegenerates | aiProcess_FindInvalidData |
                       aiProcess_GenUVCoords | aiProcess_CalcTangentSpace);

    if (!m_pScene || m_pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_pScene->mRootNode)
    {
        throw std::runtime_error("Failed to load model: " + modelPath + "\nError: " + importer.GetErrorString());
    }
    else
    {
        if (!initGeometry(m_pScene, modelPath))
        {
            throw std::runtime_error("Failed to initialize geometry for model: " + modelPath);
        }

        glm::mat4 transformation;
        getFullTransformation(m_pScene->mRootNode, m_pScene->mRootNode->mName.C_Str(), transformation);

        // ToDo: lighting calc
    }
}

} // namespace VulkanCore::model
