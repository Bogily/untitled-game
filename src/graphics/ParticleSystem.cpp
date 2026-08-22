#include "ParticleSystem.h"
#include "rlgl.h"
#include <algorithm>

ParticleEmitter::ParticleEmitter(const EmitterConfig &config)
    : config(config), emissionTimer(0.0f), active(true)
{
    particles.resize(config.maxParticles);
    for (auto &p : particles)
    {
        p.active = false;
    }
}

ParticleEmitter::~ParticleEmitter()
{
}

void ParticleEmitter::Update(float deltaTime, Vector3 camPos)
{
    if (!active)
        return;

    // Emission logic
    if (config.emissionRate > 0)
    {
        float rate = 1.0f / config.emissionRate;
        emissionTimer += deltaTime;

        while (emissionTimer > rate)
        {
            SpawnParticle();
            emissionTimer -= rate;
        }
    }

    // Update particles
    int activeCount = 0;
    for (auto &p : particles)
    {
        if (p.active)
        {
            UpdateParticle(p, deltaTime);
            if (p.active)
            {
                float dx = p.position.x - camPos.x;
                float dy = p.position.y - camPos.y;
                float dz = p.position.z - camPos.z;
                p.distanceSq = dx * dx + dy * dy + dz * dz;
                activeCount++;
            }
        }
    }

    // Sort particles back-to-front if using alpha blending
    if (activeCount > 0 && config.blendMode == ParticleBlendMode::ALPHA)
    {
        std::sort(particles.begin(), particles.end(),
                  [](const Particle &a, const Particle &b)
                  {
                      if (a.active != b.active)
                          return a.active > b.active;
                      return a.distanceSq > b.distanceSq;
                  });
    }
}

void ParticleEmitter::Burst(int count)
{
    for (int i = 0; i < count; i++)
    {
        SpawnParticle();
    }
}

void ParticleEmitter::SpawnParticle()
{
    // Find first inactive particle
    Particle *p = nullptr;
    for (auto &particle : particles)
    {
        if (!particle.active)
        {
            p = &particle;
            break;
        }
    }

    if (!p)
        return; // No free particles

    p->active = true;
    p->position = Vector3Add(config.position,
                             {(float)GetRandomValue(-100, 100) / 100.0f * config.offset.x,
                              (float)GetRandomValue(-100, 100) / 100.0f * config.offset.y,
                              (float)GetRandomValue(-100, 100) / 100.0f * config.offset.z});

    Vector3 rndVel = {
        (float)GetRandomValue(-100, 100) / 100.0f * config.velocityRandom.x,
        (float)GetRandomValue(-100, 100) / 100.0f * config.velocityRandom.y,
        (float)GetRandomValue(-100, 100) / 100.0f * config.velocityRandom.z};
    p->velocity = Vector3Add(config.velocity, rndVel);

    p->acceleration = config.acceleration;

    p->colorStart = config.colorStart;
    p->colorEnd = config.colorEnd;
    p->currentColor = p->colorStart;

    float sizeVar = (float)GetRandomValue(-100, 100) / 100.0f * config.sizeRandom;
    p->sizeStart = config.sizeStart + sizeVar;
    p->sizeEnd = config.sizeEnd + sizeVar;
    if (p->sizeStart < 0)
        p->sizeStart = 0;
    if (p->sizeEnd < 0)
        p->sizeEnd = 0;
    p->currentSize = p->sizeStart;

    p->rotation = (float)GetRandomValue(0, 360);
    p->rotationSpeed = (float)GetRandomValue(-100, 100) / 10.0f; // Random rotation speed

    p->maxLife = config.lifeMin + ((float)GetRandomValue(0, 100) / 100.0f) * (config.lifeMax - config.lifeMin);
    p->life = p->maxLife;
}

void ParticleEmitter::UpdateParticle(Particle &p, float deltaTime)
{
    p.life -= deltaTime;
    if (p.life <= 0)
    {
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
    if (particles.empty())
        return;

    // Setup texture
    if (config.texture.id > 0)
    {
        rlSetTexture(config.texture.id);
    }
    else
    {
        rlSetTexture(rlGetTextureIdDefault());
    }

    // Blending
    switch (config.blendMode)
    {
    case ParticleBlendMode::ADD:
        rlSetBlendMode(RL_BLEND_ADDITIVE);
        break;
    case ParticleBlendMode::MULTIPLY:
        rlSetBlendMode(RL_BLEND_MULTIPLIED);
        break;
    case ParticleBlendMode::SUBTRACT:
        rlSetBlendMode(RL_BLEND_SUBTRACT_COLORS);
        break;
    default:
        rlSetBlendMode(RL_BLEND_ALPHA);
        break;
    }

    rlBegin(RL_QUADS);

    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 up = camera.up;
    Vector3 right = Vector3CrossProduct(forward, up);
    up = Vector3CrossProduct(right, forward);
    right = Vector3Normalize(right);
    up = Vector3Normalize(up);

    for (const auto &p : particles)
    {
        if (!p.active)
            continue;

        rlColor4ub(p.currentColor.r, p.currentColor.g, p.currentColor.b, p.currentColor.a);

        float size = p.currentSize;
        Vector3 pPos = p.position;

        Vector3 rightVec = right;
        Vector3 upVec = up;

        if (p.rotation != 0.0f)
        {
            Matrix rotMat = MatrixRotate(forward, p.rotation * DEG2RAD);
            rightVec = Vector3Transform(right, rotMat);
            upVec = Vector3Transform(up, rotMat);
        }

        Vector3 rightScaled = Vector3Scale(rightVec, size * 0.5f);
        Vector3 upScaled = Vector3Scale(upVec, size * 0.5f);

        Vector3 p1 = Vector3Subtract(Vector3Subtract(pPos, rightScaled), upScaled);
        Vector3 p2 = Vector3Subtract(Vector3Add(pPos, rightScaled), upScaled);
        Vector3 p3 = Vector3Add(Vector3Add(pPos, rightScaled), upScaled);
        Vector3 p4 = Vector3Add(Vector3Subtract(pPos, rightScaled), upScaled);

        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(p1.x, p1.y, p1.z);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(p2.x, p2.y, p2.z);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(p3.x, p3.y, p3.z);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(p4.x, p4.y, p4.z);
    }

    rlEnd();
    rlSetBlendMode(RL_BLEND_ALPHA);
    rlSetTexture(0);
}

// ----------------------------------------------------------------------------------
// ParticleSystem Implementation
// ----------------------------------------------------------------------------------

ParticleSystem::ParticleSystem()
{
    defaultTexture = {0};
}

ParticleSystem::~ParticleSystem()
{
    Shutdown();
}

void ParticleSystem::Init()
{
    Image img = GenImageGradientRadial(32, 32, 0.0f, {255, 255, 255, 255}, {255, 255, 255, 0});
    defaultTexture = LoadTextureFromImage(img);
    textures["soft_circle"] = defaultTexture;
    UnloadImage(img);

    img = GenImageColor(32, 32, {0, 0, 0, 0});
    ImageDrawCircle(&img, 16, 16, 14, WHITE);
    textures["hard_circle"] = LoadTextureFromImage(img);
    UnloadImage(img);

    img = GenImageColor(32, 32, WHITE);
    textures["square"] = LoadTextureFromImage(img);
    UnloadImage(img);

    img = GenImageColor(32, 32, {0, 0, 0, 0});
    ImageDrawRectangle(&img, 14, 2, 4, 28, WHITE);
    ImageDrawRectangle(&img, 2, 14, 28, 4, WHITE);
    Image glow = GenImageGradientRadial(16, 16, 0.0f, WHITE, {255, 255, 255, 0});
    ImageDrawImageRec(&img, glow, {0, 0, 16, 16}, {8, 8}, WHITE);
    UnloadImage(glow);
    textures["star"] = LoadTextureFromImage(img);
    UnloadImage(img);

    img = GenImageGradientRadial(32, 32, 0.0f, {200, 200, 200, 150}, {100, 100, 100, 0});
    for (int y = 0; y < 32; y++)
    {
        for (int x = 0; x < 32; x++)
        {
            if (GetRandomValue(0, 10) > 8)
            {
                Color c = GetImageColor(img, x, y);
                c.a = (unsigned char)(c.a * 0.8f);
                ImageDrawPixel(&img, x, y, c);
            }
        }
    }
    textures["smoke"] = LoadTextureFromImage(img);
    UnloadImage(img);
}

void ParticleSystem::Shutdown()
{
    for (auto *emitter : emitters)
    {
        delete emitter;
    }
    emitters.clear();
    for (auto &pair : textures)
    {
        UnloadTexture(pair.second);
    }
    textures.clear();
    defaultTexture.id = 0;
}

void ParticleSystem::Update(float deltaTime, Vector3 camPos)
{
    for (auto *emitter : emitters)
    {
        emitter->Update(deltaTime, camPos);
    }
}

void ParticleSystem::Draw(Camera3D camera)
{
    rlDisableDepthMask();
    for (auto *emitter : emitters)
    {
        emitter->Draw(camera);
    }
    rlEnableDepthMask();
}

ParticleEmitter *ParticleSystem::CreateEmitter(const EmitterConfig &config)
{
    EmitterConfig cfg = config;
    if (cfg.texture.id == 0)
    {
        cfg.texture = defaultTexture;
    }
    ParticleEmitter *emitter = new ParticleEmitter(cfg);
    emitters.push_back(emitter);
    return emitter;
}

void ParticleSystem::RemoveEmitter(ParticleEmitter *emitter)
{
    auto it = std::find(emitters.begin(), emitters.end(), emitter);
    if (it != emitters.end())
    {
        delete *it;
        emitters.erase(it);
    }
}

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