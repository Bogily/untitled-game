
#include "Game.h"

void Game::SetupParticles(const LevelData &level)
{
    ParticleSystem *ps = renderManager.GetParticleSystem();
    if (!ps) return;

    for (const auto &pData : level.particles)
    {
        EmitterConfig config;
        config.position = pData.position;
        config.offset = pData.offset;
        config.velocity = pData.velocity;
        config.velocityRandom = pData.velocityRandom;
        config.acceleration = pData.acceleration;
        config.colorStart = pData.colorStart;
        config.colorEnd = pData.colorEnd;
        config.sizeStart = pData.sizeStart;
        config.sizeEnd = pData.sizeEnd;
        config.sizeRandom = pData.sizeRandom;
        config.lifeMin = pData.lifeMin;
        config.lifeMax = pData.lifeMax;
        config.emissionRate = pData.emissionRate;
        config.maxParticles = pData.maxParticles;
        
        // Map blend mode string to enum
        config.blendMode = ParticleBlendMode::ALPHA;
        if (pData.blendMode == "add") config.blendMode = ParticleBlendMode::ADD;
        else if (pData.blendMode == "mul") config.blendMode = ParticleBlendMode::MULTIPLY;
        else if (pData.blendMode == "sub") config.blendMode = ParticleBlendMode::SUBTRACT;
        
        // Load texture priority: Path > Name > Default
        if (!pData.texturePath.empty()) {
            config.texture = LoadTexture(pData.texturePath.c_str());
        } else if (!pData.textureName.empty()) {
            config.texture = ps->GetTexture(pData.textureName);
        } else {
            config.texture = {0}; // Use default (which ps->CreateEmitter handles by calling GetTexture("soft_circle") if 0 or similar logic, but let's be safe)
             // CreateEmitter currently checks if texture.id > 0, else uses GetTextureDefault(). 
             // But wait, I changed GetTextureDefault usage.
             // Let's explicitly set it.
             config.texture = ps->GetTexture("soft_circle"); 
        }

        ps->CreateEmitter(config);
    }

    TraceLog(LOG_INFO, "Game: Particle system setup complete with %d emitters", (int)level.particles.size());
}
