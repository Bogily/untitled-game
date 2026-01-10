#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

// Standard PBR lighting system
// Uses individual uniform uploads - scalable up to ~64 lights
// Beyond 64 lights, consider implementing deferred rendering or clustered lighting
#define LIGHT_MAX_LIGHTS 64

struct alignas(16) Light
{
    int type = 1;                 // 1 = point light, 2 = directional light
    int enabled = 1;              // 0 = disabled, 1 = enabled
    float pad0[2] = {0.0f, 0.0f}; // Padding for future UBO compatibility

    Vector4 positionRadius = {0.0f, 0.0f, 0.0f, 10.0f}; // xyz = position, w = radius
    Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float pad1[3] = {0.0f, 0.0f, 0.0f}; // Padding (total size = 64 bytes)
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
    void Update(const Camera &camera, int maxActiveLights = LIGHT_MAX_LIGHTS);

    // Light management - unified interface
    void CreatePointLight(const Vector3 &pos, const Vector4 &color, float intensity, float radius = 10.0f);
    void CreateDirectionalLight(const Vector3 &direction, const Vector4 &color, float intensity);
    void UpdateLight(int index, const Vector3 &pos, const Vector4 &color, float intensity);
    void ClearLights();
    void SetAmbientLight(const Vector3 &ambient);

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
    Vector3 ambientLight;

    // Internal methods
    void UploadLightData();
    void CullAndSortLights(const Vector3 &cameraPos, int maxActiveLights);
};
