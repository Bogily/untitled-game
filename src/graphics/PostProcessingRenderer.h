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

    // Get render texture for reading if needed
    RenderTexture2D GetSceneTexture() const { return sceneTexture; }

private:
    // Render textures
    RenderTexture2D sceneTexture; // Main scene render target

    // Shaders
    Shader grayscaleShader;

    // Screen dimensions
    int width;
    int height;

    // Effect parameters
    bool enableGrayscale;

    // Helper methods
    void RenderFullscreenQuad(Shader shader, RenderTexture2D source);
};
