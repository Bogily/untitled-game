#include "ParticleSystem.h"
#include "rlgl.h"
#include <algorithm>

// ----------------------------------------------------------------------------------
// ParticleEmitter Implementation
// ----------------------------------------------------------------------------------

ParticleEmitter::ParticleEmitter(const EmitterConfig& config)
    : config(config), emissionTimer(0.0f), active(true)
{
    particles.resize(config.maxParticles);
    for (auto& p : particles) {
        p.active = false;
    }
}

ParticleEmitter::~ParticleEmitter()
{
}

void ParticleEmitter::Update(float deltaTime, Vector3 camPos)
{
    if (!active) return;

    // Emission logic
    if (config.emissionRate > 0) {
        float rate = 1.0f / config.emissionRate;
        emissionTimer += deltaTime;
        
        while (emissionTimer > rate) {
            SpawnParticle();
            emissionTimer -= rate;
        }
    }

    // Update particles
    for (auto& p : particles) {
        if (p.active) {
            UpdateParticle(p, deltaTime);
        }
    }
    
    // Simple sort for transparency (painter's algorithm) - optional but looks better
    // This is expensive for many particles, maybe skip for now or do coarse sort?
    // Let's skip sorting for now for performance.
}

void ParticleEmitter::Burst(int count)
{
    for (int i = 0; i < count; i++) {
        SpawnParticle();
    }
}

void ParticleEmitter::SpawnParticle()
{
    // Find first inactive particle
    Particle* p = nullptr;
    for (auto& particle : particles) {
        if (!particle.active) {
            p = &particle;
            break;
        }
    }

    if (!p) return; // No free particles

    p->active = true;
    p->position = Vector3Add(config.position, 
        { (float)GetRandomValue(-100, 100) / 100.0f * config.offset.x,
          (float)GetRandomValue(-100, 100) / 100.0f * config.offset.y,
          (float)GetRandomValue(-100, 100) / 100.0f * config.offset.z });

    Vector3 rndVel = {
        (float)GetRandomValue(-100, 100) / 100.0f * config.velocityRandom.x,
        (float)GetRandomValue(-100, 100) / 100.0f * config.velocityRandom.y,
        (float)GetRandomValue(-100, 100) / 100.0f * config.velocityRandom.z
    };
    p->velocity = Vector3Add(config.velocity, rndVel);
    
    p->acceleration = config.acceleration;
    
    p->colorStart = config.colorStart;
    p->colorEnd = config.colorEnd;
    p->currentColor = p->colorStart;
    
    float sizeVar = (float)GetRandomValue(-100, 100) / 100.0f * config.sizeRandom;
    p->sizeStart = config.sizeStart + sizeVar;
    p->sizeEnd = config.sizeEnd + sizeVar;
    if (p->sizeStart < 0) p->sizeStart = 0;
    if (p->sizeEnd < 0) p->sizeEnd = 0;
    p->currentSize = p->sizeStart;
    
    p->rotation = (float)GetRandomValue(0, 360);
    p->rotationSpeed = (float)GetRandomValue(-100, 100) / 10.0f; // Random rotation speed
    
    p->maxLife = config.lifeMin + ((float)GetRandomValue(0, 100) / 100.0f) * (config.lifeMax - config.lifeMin);
    p->life = p->maxLife;
}

void ParticleEmitter::UpdateParticle(Particle& p, float deltaTime)
{
    p.life -= deltaTime;
    if (p.life <= 0) {
        p.active = false;
        return;
    }

    float t = 1.0f - (p.life / p.maxLife); // 0 to 1

    // Motion
    p.velocity = Vector3Add(p.velocity, Vector3Scale(p.acceleration, deltaTime));
    p.position = Vector3Add(p.position, Vector3Scale(p.velocity, deltaTime));
    
    // Rotation
    p.rotation += p.rotationSpeed * deltaTime;

    // Color interpolation
    p.currentColor.r = (unsigned char)(p.colorStart.r + (p.colorEnd.r - p.colorStart.r) * t);
    p.currentColor.g = (unsigned char)(p.colorStart.g + (p.colorEnd.g - p.colorStart.g) * t);
    p.currentColor.b = (unsigned char)(p.colorStart.b + (p.colorEnd.b - p.colorStart.b) * t);
    p.currentColor.a = (unsigned char)(p.colorStart.a + (p.colorEnd.a - p.colorStart.a) * t);

    // Size interpolation
    p.currentSize = p.sizeStart + (p.sizeEnd - p.sizeStart) * t;
}

void ParticleEmitter::Draw(Camera3D camera)
{
    if (particles.empty()) return;

    // Setup texture
    if (config.texture.id > 0) {
        rlSetTexture(config.texture.id);
    } else {
        // Fallback if somehow texture ID is 0 (though CreateEmitter should handle this)
        rlSetTexture(rlGetTextureIdDefault());
    }

    // Blending
    rlEnableDepthMask(); // Disable depth writing for transparent particles usually
    if (config.blending) {
        rlSetBlendMode(RL_BLEND_ADDITIVE);
    } else {
        rlSetBlendMode(RL_BLEND_ALPHA);
    }

    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255); // Default tint

    // Billboarding math
    // Get camera right and up vectors
    Vector3 camPos = camera.position;
    // Simple billboard: plane always facing camera
    // However, to do it efficiently in a loop, we can extract camera basis vectors.
    // Raylib's internal math for this:
    /*
    Vector3 lookAt = Vector3Subtract(camera.target, camera.position);
    Vector3 right = Vector3CrossProduct(lookAt, camera.up);
    Vector3 up = camera.up; // Or typically re-crossed to be orthogonal
    */
    
    // Let's use Raylib's GetCameraRight/Up if available, or compute manually.
    // Manual computation to be safe:
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 up = camera.up;
    Vector3 right = Vector3CrossProduct(forward, up);
    // Re-orthogonalize up
    up = Vector3CrossProduct(right, forward);
    
    // Normalize
    right = Vector3Normalize(right);
    up = Vector3Normalize(up);

    for (const auto& p : particles) {
        if (!p.active) continue;

        rlColor4ub(p.currentColor.r, p.currentColor.g, p.currentColor.b, p.currentColor.a);

        float size = p.currentSize;
        Vector3 pPos = p.position;

        // Calculate corners
        // If rotation is needed, we rotate right/up vectors around forward
        Vector3 rightVec = right;
        Vector3 upVec = up;

        if (p.rotation != 0.0f) {
            Matrix rotMat = MatrixRotate(forward, p.rotation * DEG2RAD);
            rightVec = Vector3Transform(right, rotMat);
            upVec = Vector3Transform(up, rotMat);
        }

        Vector3 rightScaled = Vector3Scale(rightVec, size * 0.5f);
        Vector3 upScaled = Vector3Scale(upVec, size * 0.5f);

        // Bottom-Left
        Vector3 p1 = Vector3Subtract(Vector3Subtract(pPos, rightScaled), upScaled);
        // Bottom-Right
        Vector3 p2 = Vector3Subtract(Vector3Add(pPos, rightScaled), upScaled);
        // Top-Right
        Vector3 p3 = Vector3Add(Vector3Add(pPos, rightScaled), upScaled);
        // Top-Left
        Vector3 p4 = Vector3Add(Vector3Subtract(pPos, rightScaled), upScaled);

        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(p1.x, p1.y, p1.z);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(p2.x, p2.y, p2.z);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(p3.x, p3.y, p3.z);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(p4.x, p4.y, p4.z);
    }

    rlEnd();
    
    rlSetBlendMode(RL_BLEND_ALPHA); // Reset
    rlSetTexture(0);
}

// ----------------------------------------------------------------------------------
// ParticleSystem Implementation
// ----------------------------------------------------------------------------------

ParticleSystem::ParticleSystem()
{
    defaultTexture = { 0 };
}

ParticleSystem::~ParticleSystem()
{
    Shutdown();
}

void ParticleSystem::Init()
{
    // Create a simple default texture (soft circle)
    Image img = GenImageGradientRadial(32, 32, 0.0f, {255, 255, 255, 255}, {255, 255, 255, 0});
    defaultTexture = LoadTextureFromImage(img);
    UnloadImage(img);
}

void ParticleSystem::Shutdown()
{
    for (auto* emitter : emitters) {
        delete emitter;
    }
    emitters.clear();
    
    if (defaultTexture.id > 0) {
        UnloadTexture(defaultTexture);
        defaultTexture.id = 0;
    }
}

void ParticleSystem::Update(float deltaTime, Vector3 camPos)
{
    for (auto* emitter : emitters) {
        emitter->Update(deltaTime, camPos);
    }
}

void ParticleSystem::Draw(Camera3D camera)
{
    // Disable writing to depth buffer to avoid sorting issues with transparent particles
    // (Though simple alpha blending still has sorting issues, this helps)
    rlDisableDepthMask();
    
    for (auto* emitter : emitters) {
        emitter->Draw(camera);
    }
    
    rlEnableDepthMask();
}

ParticleEmitter* ParticleSystem::CreateEmitter(const EmitterConfig& config)
{
    // Apply default texture if none provided
    EmitterConfig cfg = config;
    if (cfg.texture.id == 0) {
        cfg.texture = defaultTexture;
    }
    
    ParticleEmitter* emitter = new ParticleEmitter(cfg);
    emitters.push_back(emitter);
    return emitter;
}

void ParticleSystem::RemoveEmitter(ParticleEmitter* emitter)
{
    auto it = std::find(emitters.begin(), emitters.end(), emitter);
    if (it != emitters.end()) {
        delete *it;
        emitters.erase(it);
    }
}
