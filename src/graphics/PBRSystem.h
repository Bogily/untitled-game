#pragma once
#include "raylib.h"
#include "raymath.h"

#define PBR_MAX_LIGHTS 4

struct PBRLight
{
    int type = 1; // 1 = point light, 2 = directional light
    int enabled = 1;
    Vector3 position = {0, 0, 0}; // For point lights OR direction for directional lights
    Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
};

class PBRSystem
{
public:
    PBRSystem();
    ~PBRSystem();

    void Init();
    void Shutdown();

    bool IsInitialized() const { return initialized; }
    Shader GetShader() const { return shader; }

    // Apply PBR shader to a model
    void ApplyToModel(Model &model, const Vector4 &albedo, float metallic, float roughness);

    // Update per-frame uniforms (camera position)
    void Update(const Camera &camera);

    // Light management
    void CreatePointLight(const Vector3 &pos, const Vector4 &color, float intensity);
    void CreateDirectionalLight(const Vector3 &direction, const Vector4 &color, float intensity);
    void UpdateLight(int index, const Vector3 &pos, const Vector4 &color, float intensity);
    void DrawDebugLights();

    // Get light count
    int GetLightCount() const { return lightCount; }
    Vector3 GetSunDirection() const;

private:
    Shader shader;
    bool initialized;
    PBRLight lights[PBR_MAX_LIGHTS];
    int lightCount;

    void UploadLightData(int lightIndex);
};

// Global instance
extern PBRSystem gPBR;
