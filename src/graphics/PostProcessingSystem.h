#pragma once

#include "raylib.h"
#include "raymath.h"

class PostProcessingSystem
{
public:
    PostProcessingSystem();
    ~PostProcessingSystem();

    void Init(int screenWidth, int screenHeight);
    void Cleanup();

    // Start rendering to texture
    void BeginSceneCapture();
    void EndSceneCapture();

    // Apply all post-processing effects
    void ApplyEffects();

    // Configuration
    void SetGrayscaleEnabled(bool enable) { enableGrayscale = enable; }
    bool GetGrayscaleEnabled() const { return enableGrayscale; }

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
