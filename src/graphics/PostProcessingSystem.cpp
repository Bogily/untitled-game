#include "PostProcessingSystem.h"

PostProcessingSystem::PostProcessingSystem()
    : width(0), height(0),
      enableGrayscale(false)
{
}

PostProcessingSystem::~PostProcessingSystem()
{
    Cleanup();
}

void PostProcessingSystem::Init(int screenWidth, int screenHeight)
{
    width = screenWidth;
    height = screenHeight;

    // Create render texture
    sceneTexture = LoadRenderTexture(width, height);

    TraceLog(LOG_INFO, "PostProcessing: Render texture created (%dx%d)", width, height);

    // Load shader
    grayscaleShader = LoadShader(0, "assets/shader/grayscale.fs");

    TraceLog(LOG_INFO, "PostProcessing: Grayscale shader loaded successfully");
}

void PostProcessingSystem::Cleanup()
{
    if (sceneTexture.id > 0)
        UnloadRenderTexture(sceneTexture);
    if (grayscaleShader.id > 0)
        UnloadShader(grayscaleShader);
}

void PostProcessingSystem::BeginSceneCapture()
{
    BeginTextureMode(sceneTexture);
}

void PostProcessingSystem::EndSceneCapture()
{
    EndTextureMode();
}

void PostProcessingSystem::ApplyEffects()
{
    // Apply grayscale if enabled, otherwise just render scene texture
    if (enableGrayscale)
    {
        RenderFullscreenQuad(grayscaleShader, sceneTexture);
    }
    else
    {
        // Just render the scene texture directly
        Rectangle sourceRec = {0, 0, (float)sceneTexture.texture.width, (float)-sceneTexture.texture.height};
        Rectangle destRec = {0, 0, (float)width, (float)height};
        DrawTexturePro(sceneTexture.texture, sourceRec, destRec, {0, 0}, 0.0f, WHITE);
    }
}

void PostProcessingSystem::RenderFullscreenQuad(Shader shader, RenderTexture2D source)
{
    BeginShaderMode(shader);

    // Flip texture vertically (raylib texture coordinate system)
    Rectangle sourceRec = {0, 0, (float)source.texture.width, (float)-source.texture.height};
    Rectangle destRec = {0, 0, (float)width, (float)height};

    DrawTexturePro(source.texture, sourceRec, destRec, {0, 0}, 0.0f, WHITE);

    EndShaderMode();
}
