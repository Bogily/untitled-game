/**
 * @file ParticleSystem.h
 * @brief Particle effects system with emitters
 */

#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <unordered_map>

/**
 * @brief Particle blend modes for rendering
 */
enum class ParticleBlendMode
{
    ALPHA,    ///< Alpha blending
    ADD,      ///< Additive blending
    MULTIPLY, ///< Multiplicative blending
    SUBTRACT  ///< Subtractive blending
};

/**
 * @brief Individual particle instance
 */
struct Particle
{
    Vector3 position;     ///< World position
    Vector3 velocity;     ///< Velocity vector
    Vector3 acceleration; ///< Acceleration vector

    Color colorStart;   ///< Initial color
    Color colorEnd;     ///< Final color
    Color currentColor; ///< Current interpolated color

    float sizeStart;   ///< Initial size
    float sizeEnd;     ///< Final size
    float currentSize; ///< Current interpolated size

    float rotation;      ///< Current rotation angle
    float rotationSpeed; ///< Rotation speed

    float life;    ///< Remaining lifetime
    float maxLife; ///< Total lifetime

    float distanceSq; ///< Distance squared to camera (for sorting)
    bool active;      ///< Whether particle is active
};

/**
 * @brief Particle emitter configuration
 */
struct EmitterConfig
{
    Vector3 position; ///< Emitter world position
    Vector3 offset;   ///< Random spawn offset range

    Vector3 velocity;       ///< Base particle velocity
    Vector3 velocityRandom; ///< Random velocity variance

    Vector3 acceleration; ///< Particle acceleration

    Color colorStart; ///< Particle start color
    Color colorEnd;   ///< Particle end color

    float sizeStart;  ///< Particle start size
    float sizeEnd;    ///< Particle end size
    float sizeRandom; ///< Random size variance

    float lifeMin; ///< Minimum particle lifetime
    float lifeMax; ///< Maximum particle lifetime

    float emissionRate; ///< Particles spawned per second
    int maxParticles;   ///< Maximum active particles

    ParticleBlendMode blendMode; ///< Rendering blend mode
    Texture2D texture;           ///< Particle texture (id=0 uses default)
};

/**
 * @brief Particle emitter managing particle spawning and lifecycle
 */
class ParticleEmitter
{
public:
    /**
     * @brief Construct emitter with configuration
     * @param config Emitter configuration
     */
    ParticleEmitter(const EmitterConfig &config);

    /**
     * @brief Cleanup emitter resources
     */
    ~ParticleEmitter();

    /**
     * @brief Update all particles and spawn new ones
     * @param deltaTime Time elapsed since last frame
     * @param camPos Camera position for sorting
     */
    void Update(float deltaTime, Vector3 camPos);

    /**
     * @brief Render all active particles
     * @param camera Camera for billboarding
     */
    void Draw(Camera3D camera);

    /**
     * @brief Set emitter world position
     * @param pos New position
     */
    void SetPosition(Vector3 pos) { config.position = pos; }

    /**
     * @brief Enable or disable emitter
     * @param active Active state
     */
    void SetActive(bool active) { this->active = active; }

    /**
     * @brief Spawn burst of particles immediately
     * @param count Number of particles to spawn
     */
    void Burst(int count);

private:
    /**
     * @brief Spawn single particle
     */
    void SpawnParticle();

    /**
     * @brief Update single particle physics and lifetime
     * @param p Particle to update
     * @param deltaTime Time elapsed
     */
    void UpdateParticle(Particle &p, float deltaTime);

    EmitterConfig config;            ///< Emitter configuration
    std::vector<Particle> particles; ///< All particles (active + inactive)
    float emissionTimer;             ///< Time until next particle spawn
    bool active;                     ///< Whether emitter is spawning particles

    friend class ParticleSystem; ///< Allow system access to internals
};

/**
 * @brief Particle system manager coordinating multiple emitters
 */
class ParticleSystem
{
public:
    /**
     * @brief Construct particle system
     */
    ParticleSystem();

    /**
     * @brief Cleanup all emitters and resources
     */
    ~ParticleSystem();

    /**
     * @brief Initialize particle system
     */
    void Init();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Update all emitters
     * @param deltaTime Time elapsed since last frame
     * @param camPos Camera position for particle sorting
     */
    void Update(float deltaTime, Vector3 camPos);

    /**
     * @brief Render all emitters
     * @param camera Camera for billboarding
     */
    void Draw(Camera3D camera);

    /**
     * @brief Create new particle emitter
     * @param config Emitter configuration
     * @return Pointer to created emitter
     */
    ParticleEmitter *CreateEmitter(const EmitterConfig &config);

    /**
     * @brief Remove and destroy emitter
     * @param emitter Emitter to remove
     */
    void RemoveEmitter(ParticleEmitter *emitter);

    /**
     * @brief Load and cache texture by name
     * @param name Texture identifier
     * @return Loaded texture
     */
    Texture2D GetTexture(const std::string &name);

private:
    std::vector<ParticleEmitter *> emitters;             ///< All active emitters
    std::unordered_map<std::string, Texture2D> textures; ///< Cached particle textures
    Texture2D defaultTexture;                            ///< Default particle texture
};