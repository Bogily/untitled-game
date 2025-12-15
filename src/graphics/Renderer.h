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

    // Simple SSAO post-process approach
    // Call BeginSceneCapture before drawing 3D scene
    void BeginSceneCapture();
    // Call EndSceneCapture after drawing 3D scene
    void EndSceneCapture();

    // Apply SSAO and render final image to screen
    void ApplySSAOAndRender(Camera3D &camera);

    // Debug visualization
    void DrawDebugBuffer(int bufferIndex, Camera3D &camera); // 0=color, 1=depth, 2=ssao, 3=normals

    // Settings
    void SetSSAOEnabled(bool enabled) { ssaoEnabled = enabled; }
    bool IsSSAOEnabled() const { return ssaoEnabled; }

    void SetSSAORadius(float r) { ssaoRadius = r; }
    float GetSSAORadius() const { return ssaoRadius; }

    void SetSSAOBias(float b) { ssaoBias = b; }
    float GetSSAOBias() const { return ssaoBias; }

    void SetSSAOIntensity(float i) { ssaoIntensity = i; }
    float GetSSAOIntensity() const { return ssaoIntensity; }

    void SetSSAOSamples(int s) { ssaoKernelSize = s; }
    int GetSSAOSamples() const { return ssaoKernelSize; }

private:
    int screenWidth;
    int screenHeight;

    // Scene capture (color + depth)
    RenderTexture2D sceneTexture;

    // SSAO
    unsigned int ssaoFBO;
    unsigned int ssaoColorBuffer;
    unsigned int ssaoBlurFBO;
    unsigned int ssaoBlurColorBuffer;
    unsigned int noiseTexture;
    std::vector<Vector3> ssaoKernel;

    // Shaders
    Shader ssaoShader;
    Shader ssaoBlurShader;
    Shader compositeShader;
    Shader debugShader;

    // Fullscreen quad
    unsigned int quadVAO;
    unsigned int quadVBO;

    // Settings
    bool ssaoEnabled;
    float ssaoRadius;
    float ssaoBias;
    float ssaoIntensity;
    int ssaoKernelSize;

    // Private methods
    void CreateSceneTexture();
    void CreateSSAOBuffers();
    void CreateFullscreenQuad();
    void GenerateSSAOKernel();
    void GenerateNoiseTexture();
    void LoadShaders();
    void RenderQuad();

    float Lerp(float a, float b, float t) { return a + t * (b - a); }
};
