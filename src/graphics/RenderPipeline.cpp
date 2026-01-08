#include "RenderPipeline.h"
#include "PostProcessingSystem.h"
#include "ForwardPlusSystem.h"

RenderPipeline::RenderPipeline()
    : screenWidth(0),
      screenHeight(0),
      postProcessingEnabled(true),
      forwardPlusEnabled(false),
      initialized(false),
      postProcessing(nullptr),
      forwardPlus(nullptr),
      sunDirection({0.3f, 0.5f, 0.8f})
{
}

RenderPipeline::~RenderPipeline()
{
    Shutdown();
}

void RenderPipeline::Init(int width, int height)
{
    if (initialized)
    {
        TraceLog(LOG_WARNING, "RenderPipeline: Already initialized");
        return;
    }

    screenWidth = width;
    screenHeight = height;

    // Create post-processing system
    postProcessing = new PostProcessingSystem();
    postProcessing->Init(width, height);

    // Create Forward+ system
    forwardPlus = new ForwardPlusSystem();
    forwardPlus->Init(width, height);

    initialized = true;
    TraceLog(LOG_INFO, "RenderPipeline: Initialized (%dx%d)", width, height);
}

void RenderPipeline::Shutdown()
{
    if (postProcessing)
    {
        postProcessing->Cleanup();
        delete postProcessing;
        postProcessing = nullptr;
    }

    if (forwardPlus)
    {
        forwardPlus->Shutdown();
        delete forwardPlus;
        forwardPlus = nullptr;
    }

    initialized = false;
    TraceLog(LOG_INFO, "RenderPipeline: Shutdown complete");
}

void RenderPipeline::BeginFrame()
{
    // BeginDrawing is called by the main game loop before this
    // We just clear the background here
    ClearBackground(BLACK);
}

void RenderPipeline::RenderScene(std::function<void()> sceneRenderer, Camera3D camera)
{
    if (!initialized)
    {
        TraceLog(LOG_ERROR, "RenderPipeline: Not initialized!");
        return;
    }

    currentCamera = camera;

    // Choose rendering path based on post-processing state
    if (postProcessingEnabled && IsPostProcessingReady())
    {
        RenderWithPostProcessing(sceneRenderer);
    }
    else
    {
        RenderDirect(sceneRenderer);
    }
}

void RenderPipeline::EndFrame()
{
    // EndDrawing is called by the main game loop after this
    // Nothing to do here for now
}

void RenderPipeline::RenderDirect(std::function<void()> sceneRenderer)
{
    // Simple direct rendering - no post-processing
    // Screen is already cleared by BeginFrame()

    // Execute the scene rendering callback
    sceneRenderer();
}

void RenderPipeline::RenderWithPostProcessing(std::function<void()> sceneRenderer)
{
    // Render scene to texture
    postProcessing->BeginSceneCapture();
    ClearBackground(BLACK);

    // Execute the scene rendering callback
    sceneRenderer();

    postProcessing->EndSceneCapture();

    // Clear screen before applying post-processing
    ClearBackground(BLACK);

    // Apply post-processing effects
    postProcessing->ApplyEffects();
}

bool RenderPipeline::IsPostProcessingReady() const
{
    // Check if post-processing system is properly initialized
    return postProcessing != nullptr;
}
