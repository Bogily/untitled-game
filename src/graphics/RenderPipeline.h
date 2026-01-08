#pragma once

#include "raylib.h"
#include "raymath.h"
#include <functional>

// Forward declarations
class PostProcessingSystem;
class ForwardPlusSystem;

// Render pipeline manages the flow of rendering from scene to screen
// with optional post-processing effects
class RenderPipeline
{
public:
    RenderPipeline();
    ~RenderPipeline();

    void Init(int width, int height);
    void Shutdown();

    // Enable/disable post-processing (can toggle at runtime)
    void EnablePostProcessing(bool enable) { postProcessingEnabled = enable; }
    bool IsPostProcessingEnabled() const { return postProcessingEnabled; }

    // Enable/disable Forward+ rendering (can toggle at runtime)
    void EnableForwardPlus(bool enable) { forwardPlusEnabled = enable; }
    bool IsForwardPlusEnabled() const { return forwardPlusEnabled; }

    // Main rendering entry point
    // Call BeginFrame() once at start of frame
    // Call RenderScene() with a callback that renders your 3D scene
    // Call EndFrame() to finalize
    void BeginFrame();
    void RenderScene(std::function<void()> sceneRenderer, Camera3D camera);
    void EndFrame();

    // Get post-processing system for configuration
    PostProcessingSystem *GetPostProcessing() { return postProcessing; }

    // Get Forward+ system for configuration
    ForwardPlusSystem *GetForwardPlus() { return forwardPlus; }

    // Sun configuration
    void SetSunDirection(Vector3 direction) { sunDirection = direction; }
    Vector3 GetSunDirection() const { return sunDirection; }

    // Get if initialized properly
    bool IsInitialized() const { return initialized; }

private:
    int screenWidth;
    int screenHeight;
    bool postProcessingEnabled;
    bool forwardPlusEnabled;
    bool initialized;

    PostProcessingSystem *postProcessing;
    ForwardPlusSystem *forwardPlus;

    Vector3 sunDirection;
    Camera3D currentCamera;

    // Internal rendering methods
    void RenderDirect(std::function<void()> sceneRenderer);
    void RenderWithPostProcessing(std::function<void()> sceneRenderer);

    // Check if post-processing is ready
    bool IsPostProcessingReady() const;
};
