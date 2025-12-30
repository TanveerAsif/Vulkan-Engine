#include "Material.h"

namespace VulkanCore::model
{

CoreMaterial::~CoreMaterial()
{
    for (Texture* pTex : mpTextures)
    {
        delete pTex;
    }
}

} // namespace VulkanCore::model
