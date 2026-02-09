#pragma once

#include "raylib.h"
#include "raymath.h"

class PostProcessingRenderer
{
public:
    PostProcessingRenderer();
    ~PostProcessingRenderer();

    void Init(int screenWidth, int screenHeight);
    void Shutdown();

    // Start rendering to texture
    void BeginSceneCapture();
    void EndSceneCapture();

    // Apply all post-processing effects
    void ApplyEffects();

    // Configuration
    void SetGrayscaleEnabled(bool enable) { enableGrayscale = enable; }
    bool GetGrayscaleEnabled() const { return enableGrayscale; }
    
    void SetDepthDebugEnabled(bool enable) { enableDepthDebug = enable; }
    bool GetDepthDebugEnabled() const { return enableDepthDebug; }

    // Get render texture for reading if needed
    RenderTexture2D GetSceneTexture() const { return sceneTexture; }

private:
    // Render textures
    RenderTexture2D sceneTexture; // Main scene render target with depth texture attached

    // Shaders
    Shader grayscaleShader;
    Shader depthShader;

    // Screen dimensions
    int width;
    int height;

    // Effect parameters
    bool enableGrayscale;
    bool enableDepthDebug;

    // Helper methods
    void RenderFullscreenQuad(Shader shader, RenderTexture2D source);
    RenderTexture2D LoadRenderTextureWithDepth(int screenWidth, int screenHeight);
    void UnloadRenderTextureWithDepth(RenderTexture2D target);
};
