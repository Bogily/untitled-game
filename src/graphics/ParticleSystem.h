#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>

struct Particle {
    Vector3 position;
    Vector3 velocity;
    Vector3 acceleration;
    
    Color colorStart;
    Color colorEnd;
    Color currentColor;
    
    float sizeStart;
    float sizeEnd;
    float currentSize;
    
    float rotation;
    float rotationSpeed;
    
    float life;      // Remaining life
    float maxLife;   // Total life duration
    
    bool active;
};

struct EmitterConfig {
    Vector3 position;
    Vector3 offset; // Random offset range
    
    Vector3 velocity;
    Vector3 velocityRandom; // Random velocity range
    
    Vector3 acceleration;
    
    Color colorStart;
    Color colorEnd;
    
    float sizeStart;
    float sizeEnd;
    float sizeRandom; // Random size variance
    
    float lifeMin;
    float lifeMax;
    
    float emissionRate; // Particles per second
    int maxParticles;
    
    bool blending; // Additive blending if true
    Texture2D texture;
};

class ParticleEmitter {
public:
    ParticleEmitter(const EmitterConfig& config);
    ~ParticleEmitter();
    
    void Update(float deltaTime, Vector3 camPos);
    void Draw(Camera3D camera);
    
    void SetPosition(Vector3 pos) { config.position = pos; }
    void SetActive(bool active) { this->active = active; }
    
    // Helper to burst particles
    void Burst(int count);

private:
    void SpawnParticle();
    void UpdateParticle(Particle& p, float deltaTime);

    EmitterConfig config;
    std::vector<Particle> particles;
    float emissionTimer;
    bool active;
    
    // Pre-allocated vertex buffers for batching could be added here
    // For now, we will use rlgl immediate mode or DrawBillboard
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();
    
    void Init();
    void Shutdown();
    
    void Update(float deltaTime, Vector3 camPos);
    void Draw(Camera3D camera);
    
    ParticleEmitter* CreateEmitter(const EmitterConfig& config);
    void RemoveEmitter(ParticleEmitter* emitter);

private:
    std::vector<ParticleEmitter*> emitters;
    Texture2D defaultTexture;
};
