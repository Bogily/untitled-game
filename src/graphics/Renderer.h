#pragma once
#include "raylib.h"
#include "rlgl.h"
#include <vector>
#include <random>
#include <cmath>

// OpenGL function loading - use raylib's external GLAD
#if defined(_WIN32)
#define GLAD_API_CALL __declspec(dllimport)
#endif
#include "external/glad.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    // Initialize renderer with screen dimensions
    void Init(int width, int height);

    // Cleanup resources
    void Shutdown();

    // Render scene to texture
    void BeginSceneCapture();
    void EndSceneCapture();

    // Apply fog and render final image to screen
    void ApplyFogAndRender(Camera3D &camera);

    // Debug visualization
    void DrawDebugBuffer(int bufferIndex, Camera3D &camera); // 0=color, 1=depth, 2=normals

    // Settings
    void SetFogEnabled(bool enabled) { fogEnabled = enabled; }
    bool IsFogEnabled() const { return fogEnabled; }

    void SetFogDistance(float d) { fogDistance = d; }
    float GetFogDistance() const { return fogDistance; }

    void SetFogDensity(float d) { fogDensity = d; }
    float GetFogDensity() const { return fogDensity; }

private:
    int screenWidth;
    int screenHeight;

    // Scene capture (color + depth)
    RenderTexture2D sceneTexture;

    // Shaders
    Shader compositeShader;
    Shader debugShader;

    // Fullscreen quad
    unsigned int quadVAO;
    unsigned int quadVBO;

    // Settings
    bool fogEnabled;
    float fogDistance;
    float fogDensity;

    // Private methods
    void CreateSceneTexture();
    void CreateFullscreenQuad();
    void LoadShaders();
    void RenderQuad();
};