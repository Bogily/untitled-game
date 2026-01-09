#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

// Standard PBR lighting system
#define LIGHT_MAX_LIGHTS 4

struct alignas(16) Light
{
    int type = 1;                 // 1 = point light, 2 = directional light
    int enabled = 1;              // bool stored as int for GPU compatibility
    float pad0[2] = {0.0f, 0.0f}; // Align next member to 16 bytes for std430

    // Pack position.xyz and radius into a single 16-byte block (std430/vec4 aligned)
    Vector4 positionRadius = {0.0f, 0.0f, 0.0f, 10.0f};

    Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float pad1[3] = {0.0f, 0.0f, 0.0f}; // Total struct size = 64 bytes
};

// static_assert(sizeof(Light) == 64, "Light must match std430 layout (64 bytes)");

class LightRenderer
{
public:
    LightRenderer();
    ~LightRenderer();

    void Init(int screenWidth, int screenHeight);
    void Shutdown();

    bool IsInitialized() const { return initialized; }

    // Apply shader to a model
    void ApplyToModel(Model &model, const Vector4 &albedo, float metallic, float roughness);

    // Update per-frame uniforms (camera position and light data)
    void Update(const Camera &camera);

    // Light management - unified interface
    void CreatePointLight(const Vector3 &pos, const Vector4 &color, float intensity, float radius = 10.0f);
    void CreateDirectionalLight(const Vector3 &direction, const Vector4 &color, float intensity);
    void UpdateLight(int index, const Vector3 &pos, const Vector4 &color, float intensity);
    void ClearLights();

    // Get light count and info
    int GetLightCount() const { return lightCount; }
    Vector3 GetSunDirection() const;

    // Debug visualization
    void DrawDebugLights();

private:
    Shader pbrShader;

    bool initialized;

    std::vector<Light> lights;
    int lightCount;

    // Internal methods
    void UploadLightData();
};
