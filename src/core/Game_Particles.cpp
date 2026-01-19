
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
        config.blending = pData.blending;
        
        // Load texture if specified
        if (!pData.texturePath.empty()) {
            config.texture = LoadTexture(pData.texturePath.c_str());
        } else {
            config.texture = {0}; // Use default
        }

        ps->CreateEmitter(config);
    }

    TraceLog(LOG_INFO, "Game: Particle system setup complete with %d emitters", (int)level.particles.size());
}
