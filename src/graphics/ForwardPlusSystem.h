#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

#define FORWARD_PLUS_TILE_SIZE 16
#define FORWARD_PLUS_MAX_LIGHTS 128
#define FORWARD_PLUS_MAX_LIGHTS_PER_TILE 256

struct ForwardPlusLight
{
    int type = 1; // 1 = point light, 2 = directional light
    int enabled = 1;
    Vector3 position = {0, 0, 0}; // For point lights OR direction for directional lights
    float radius = 10.0f;         // Light radius for culling (point lights only)
    Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float padding[2] = {0, 0}; // Alignment padding for std430
};

class ForwardPlusSystem
{
public:
    ForwardPlusSystem();
    ~ForwardPlusSystem();

    void Init(int screenWidth, int screenHeight);
    void Shutdown();

    bool IsInitialized() const { return initialized; }
    Shader GetShader() const { return pbrShader; }

    // Apply Forward+ PBR shader to a model
    void ApplyToModel(Model &model, const Vector4 &albedo, float metallic, float roughness);

    // Update per-frame uniforms and perform light culling
    void Update(const Camera &camera);

    // Light management
    void CreatePointLight(const Vector3 &pos, const Vector4 &color, float intensity, float radius = 10.0f);
    void CreateDirectionalLight(const Vector3 &direction, const Vector4 &color, float intensity);
    void UpdateLight(int index, const Vector3 &pos, const Vector4 &color, float intensity);
    void ClearLights();

    // Get light count
    int GetLightCount() const { return lightCount; }

    // Perform light culling (called automatically in Update)
    void PerformLightCulling(const Camera &camera);

    // Debug visualization
    void DrawDebugLights();

private:
    Shader pbrShader;
    Shader lightCullingShader;
    bool initialized;

    std::vector<ForwardPlusLight> lights;
    int lightCount;

    int screenWidth;
    int screenHeight;
    int numTilesX;
    int numTilesY;

    // GPU buffers for Forward+ light culling
    unsigned int lightBuffer;               // SSBO for light data
    unsigned int visibleLightIndicesBuffer; // SSBO for visible light indices per tile
    unsigned int lightGridBuffer;           // SSBO for light grid (offset + count per tile)

    void CreateComputeBuffers();
    void DestroyComputeBuffers();
    void UploadLightData();
    void UpdateShaderUniforms(const Camera &camera);
};

// Global instance
extern ForwardPlusSystem gForwardPlus;
