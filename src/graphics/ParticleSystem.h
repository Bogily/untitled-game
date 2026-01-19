#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <unordered_map>

enum class ParticleBlendMode {
    ALPHA,
    ADD,
    MULTIPLY,
    SUBTRACT
};

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
    
    float distanceSq; // Distance squared to camera (for sorting)
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
    
    ParticleBlendMode blendMode;
    Texture2D texture; // If id=0, uses default
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
    
    friend class ParticleSystem;
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
    
    Texture2D GetTexture(const std::string& name);

private:
    std::vector<ParticleEmitter*> emitters;
    std::unordered_map<std::string, Texture2D> textures;
    Texture2D defaultTexture;
};