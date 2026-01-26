#include "ParticleSystem.h"

Texture2D ParticleSystem::GetTexture(const std::string &name)
{
    auto it = textures.find(name);
    if (it != textures.end())
    {
        return it->second;
    }
    // Return default texture if not found
    return defaultTexture;
}
