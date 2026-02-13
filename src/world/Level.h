/**
 * @file Level.h
 * @brief Level data structures and scene configuration
 */

#pragma once
#include "raylib.h"
#include <vector>
#include <string>

/**
 * @brief Pure scene data container without rendering or update logic
 *
 * Stores all data needed to configure a game level including objects,
 * NPCs, lights, particles, and environment settings.
 */
class LevelData
{
public:
    /**
     * @brief Static object data
     */
    struct ObjectData
    {
        enum class Mobility
        {
            Static,
            Dynamic
        };

        std::string name;        ///< Object name
        Vector3 position;        ///< World position
        Vector3 rotation;        ///< Euler rotation
        Vector3 scale;           ///< Scale factors
        Color albedo = WHITE;    ///< Base color
        float metallic = -1.0f;  ///< Metallic factor (-1.0 = use model default)
        float roughness = -1.0f; ///< Roughness factor (-1.0 = use model default)
        std::string modelType;   ///< Model type: "sphere", "cube", "cylinder", etc.
        Mobility mobility = Mobility::Static; ///< Object mobility for scene runtime (static/dynamic)
    };

    /**
     * @brief NPC data and dialogue configuration
     */
    struct NPCData
    {
        std::string name;                       ///< NPC name
        Vector3 position;                       ///< World position
        std::vector<std::string> dialogueLines; ///< Dialogue sequence
        float interactionRange;                 ///< Interaction distance
    };

    /**
     * @brief Light source configuration
     */
    struct LightData
    {
        std::string name;  ///< Light name
        int type;          ///< 0=directional, 1=point
        Vector3 position;  ///< World position (point lights)
        Vector3 direction; ///< Light direction (directional lights)
        Color color;       ///< Light color
        float intensity;   ///< Light intensity
        float radius;      ///< Light radius (point lights)
    };

    /**
     * @brief Camera configuration
     */
    struct CameraData
    {
        Vector3 startPosition; ///< Initial camera position
        Vector3 startTarget;   ///< Initial look-at target
        float startFOV;        ///< Initial field of view
        float followDistance;  ///< Camera follow distance from player
        float followHeight;    ///< Camera height above player
        float smoothness;      ///< Camera smoothing factor
    };

    /**
     * @brief Particle emitter configuration
     */
    struct ParticleEmitterData
    {
        Vector3 position;        ///< Emitter position
        Vector3 offset;          ///< Random spawn offset
        Vector3 velocity;        ///< Base particle velocity
        Vector3 velocityRandom;  ///< Velocity randomization
        Vector3 acceleration;    ///< Particle acceleration
        Color colorStart;        ///< Initial particle color
        Color colorEnd;          ///< Final particle color
        float sizeStart;         ///< Initial particle size
        float sizeEnd;           ///< Final particle size
        float sizeRandom;        ///< Size randomization
        float lifeMin;           ///< Minimum particle lifetime
        float lifeMax;           ///< Maximum particle lifetime
        float emissionRate;      ///< Particles per second
        int maxParticles;        ///< Maximum active particles
        std::string blendMode;   ///< "alpha", "add", "mul", "sub"
        std::string textureName; ///< Built-in texture name
        std::string texturePath; ///< Optional: load from file
    };

    std::string name;                           ///< Level name
    CameraData camera;                          ///< Camera configuration
    Vector3 playerStartPosition;                ///< Player spawn position
    std::vector<ObjectData> objects;            ///< Scene object definitions
    std::vector<NPCData> npcs;                  ///< Non-player characters
    std::vector<LightData> lights;              ///< Light sources
    std::vector<ParticleEmitterData> particles; ///< Particle emitters

    Vector3 grassPosition = {0.0f, 0.0f, 0.0f}; ///< Grass area position
    float grassWidth = 0.0f;                    ///< Grass area width
    float grassLength = 0.0f;                   ///< Grass area length
    int grassBladeCount = 0;                    ///< Number of grass blades

    Vector3 waterPosition = {0.0f, 0.0f, 0.0f}; ///< Water surface position
    float waterWidth = 0.0f;                    ///< Water surface width
    float waterLength = 0.0f;                   ///< Water surface length

    std::string skyboxTexture; ///< Skybox texture path

    float gravity = 9.8f; ///< Physics gravity

    /**
     * @brief Construct default level
     */
    LevelData() : name("Untitled Level") {}

    /**
     * @brief Create test level with sample data
     * @return Test level configuration
     */
    static LevelData CreateTestLevel();
};
