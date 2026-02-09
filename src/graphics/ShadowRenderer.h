#pragma once

#include "raylib.h"
#include "raymath.h"

/**
 * Shadow rendering system for realistic shadow mapping.
 * Renders the scene from the light's perspective to a depth texture.
 */
class ShadowRenderer
{
public:
    ShadowRenderer();
    ~ShadowRenderer();

    void Init(int shadowMapWidth = 4096, int shadowMapHeight = 4096);
    void Shutdown();

    // Shadow map rendering
    void BeginShadowPass(Vector3 lightPos, Vector3 lightTarget, Vector3 lightUp);
    void EndShadowPass();

    // Configuration
    void SetLightDirection(Vector3 direction) { lightDirection = direction; }
    void SetShadowBias(float bias) { shadowBias = bias; }
    void SetShadowEnabled(bool enable) { enableShadows = enable; }

    bool IsShadowEnabled() const { return enableShadows; }
    float GetShadowBias() const { return shadowBias; }

    // Get shadow map and light space matrix for shader use
    Texture2D GetShadowMap() const { return shadowMap.depth; }
    Texture2D GetShadowMapColor() const { return shadowMap.texture; }
    Matrix GetLightSpaceMatrix() const { return lightSpaceMatrix; }
    Vector3 GetLightDirection() const { return lightDirection; }

    // Debug visualization
    void DebugDrawShadowMap(int x, int y, int width, int height);

private:
    // Shadow map framebuffer
    RenderTexture2D shadowMap;

    // Shader for depth rendering
    Shader depthShader;

    // Light space matrix for transforming world space to light space
    Matrix lightSpaceMatrix;

    // Shadow parameters
    Vector3 lightDirection;
    float shadowBias;
    bool enableShadows;

    int shadowMapWidth;
    int shadowMapHeight;

    // Helper methods
    RenderTexture2D LoadShadowMap(int width, int height);
    void UnloadShadowMap(RenderTexture2D &target);
    Matrix CalculateLightSpaceMatrix(Vector3 lightPos, Vector3 lightTarget, Vector3 lightUp);
};
