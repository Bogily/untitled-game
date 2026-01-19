#pragma once
#include "raylib.h"
#include <vector>
#include <string>

namespace World
{
    // Forward declarations
    class WorldManager;
}

// Stores pure scene data - no rendering or update logic
class LevelData
{
public:
    struct ObjectData
    {
        std::string name;
        Vector3 position;
        Vector3 rotation;
        Vector3 scale;
        Color albedo;
        float metallic;
        float roughness;
        std::string modelType; // "sphere", "cube", "cylinder", etc.
        Vector3 collisionSize;
        float collisionRadius;
        float collisionHeight;
        std::string collisionType; // "box", "sphere", "cylinder", "capsule", "none"
    };

    struct NPCData
    {
        std::string name;
        Vector3 position;
        std::vector<std::string> dialogueLines;
        float interactionRange;
    };

    struct LightData
    {
        std::string name;
        int type; // 0=directional, 1=point
        Vector3 position;
        Vector3 direction;
        Color color;
        float intensity;
        float radius;
    };

    struct CameraData
    {
        Vector3 startPosition;
        Vector3 startTarget;
        float startFOV;
        float followDistance;
        float followHeight;
        float smoothness;
    };

    struct ParticleEmitterData
    {
        Vector3 position;
        Vector3 offset;
        Vector3 velocity;
        Vector3 velocityRandom;
        Vector3 acceleration;
        Color colorStart;
        Color colorEnd;
        float sizeStart;
        float sizeEnd;
        float sizeRandom;
        float lifeMin;
        float lifeMax;
        float emissionRate;
        int maxParticles;
        std::string blendMode; // "alpha", "add", "mul", "sub"
        std::string textureName; // "soft_circle", "star", etc. or path
        std::string texturePath; // Optional: load from file if not built-in
    };

    // Level data
    std::string name;
    CameraData camera;
    Vector3 playerStartPosition;
    std::vector<ObjectData> objects;
    std::vector<NPCData> npcs;
    std::vector<LightData> lights;
    std::vector<ParticleEmitterData> particles;

    // Grass/Water settings
    Vector3 grassPosition;
    float grassWidth;
    float grassLength;
    int grassBladeCount;

    Vector3 waterPosition;
    float waterWidth;
    float waterLength;

    // Skybox settings
    std::string skyboxTexture;

    // Physics settings
    float gravity = 9.8f;

    LevelData() : name("Untitled Level") {}

    // Factory method to create the test scene level
    static LevelData CreateTestLevel();
};
