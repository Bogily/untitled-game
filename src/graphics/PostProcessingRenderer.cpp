#include "PostProcessingRenderer.h"
#include "rlgl.h"

PostProcessingRenderer::PostProcessingRenderer()
    : width(0), height(0),
      enableGrayscale(false),
      enableDepthDebug(false)
{
}

PostProcessingRenderer::~PostProcessingRenderer()
{
    Shutdown();
}

void PostProcessingRenderer::Init(int screenWidth, int screenHeight)
{
    width = screenWidth;
    height = screenHeight;

    // Create render texture with depth texture attached
    sceneTexture = LoadRenderTextureWithDepth(width, height);

    TraceLog(LOG_INFO, "PostProcessingRenderer: Render texture created with depth (%dx%d)", width, height);

    // Load shaders
    grayscaleShader = LoadShader(0, "assets/shader/grayscale.fs");
    depthShader = LoadShader(0, "assets/shader/depth_render.fs");

    TraceLog(LOG_INFO, "PostProcessingRenderer: Grayscale shader loaded successfully");
    TraceLog(LOG_INFO, "PostProcessingRenderer: Depth render shader loaded successfully");
}

void PostProcessingRenderer::Shutdown()
{
    if (sceneTexture.id > 0)
        UnloadRenderTextureWithDepth(sceneTexture);
    if (grayscaleShader.id > 0)
        UnloadShader(grayscaleShader);
    if (depthShader.id > 0)
        UnloadShader(depthShader);

    TraceLog(LOG_INFO, "PostProcessingRenderer: Shutdown complete");
}

void PostProcessingRenderer::BeginSceneCapture()
{
    BeginTextureMode(sceneTexture);
}

void PostProcessingRenderer::EndSceneCapture()
{
    EndTextureMode();
}

void PostProcessingRenderer::ApplyEffects()
{
    if (enableDepthDebug)
    {
        // Render depth buffer with shader
        BeginShaderMode(depthShader);
        int depthLoc = GetShaderLocation(depthShader, "depthTexture");
        int flipTextureLoc = GetShaderLocation(depthShader, "flipY");
        SetShaderValue(depthShader, flipTextureLoc, (int[]){1}, SHADER_UNIFORM_INT);
        SetShaderValueTexture(depthShader, depthLoc, sceneTexture.depth);

        // Render fullscreen quad with depth texture
        Rectangle sourceRec = {0, 0, (float)sceneTexture.depth.width, (float)-sceneTexture.depth.height};
        Rectangle destRec = {0, 0, (float)width, (float)height};
        DrawTexturePro(sceneTexture.depth, sourceRec, destRec, {0, 0}, 0.0f, WHITE);

        EndShaderMode();
    }
    else if (enableGrayscale)
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

void PostProcessingRenderer::RenderFullscreenQuad(Shader shader, RenderTexture2D source)
{
    BeginShaderMode(shader);

    // Flip texture vertically (raylib texture coordinate system)
    Rectangle sourceRec = {0, 0, (float)source.texture.width, (float)-source.texture.height};
    Rectangle destRec = {0, 0, (float)width, (float)height};

    DrawTexturePro(source.texture, sourceRec, destRec, {0, 0}, 0.0f, WHITE);

    EndShaderMode();
}

RenderTexture2D PostProcessingRenderer::LoadRenderTextureWithDepth(int screenWidth, int screenHeight)
{
    RenderTexture2D target = {0};

    target.id = rlLoadFramebuffer(); // Load an empty framebuffer

    if (target.id > 0)
    {
        rlEnableFramebuffer(target.id);

        // Create color texture (default to RGBA)
        target.texture.id = rlLoadTexture(0, screenWidth, screenHeight, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        target.texture.width = screenWidth;
        target.texture.height = screenHeight;
        target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        target.texture.mipmaps = 1;

        // Create depth texture buffer (instead of raylib default renderbuffer)
        target.depth.id = rlLoadTextureDepth(screenWidth, screenHeight, false);
        target.depth.width = screenWidth;
        target.depth.height = screenHeight;
        target.depth.format = 19; // DEPTH_COMPONENT_24BIT: Not defined in raylib
        target.depth.mipmaps = 1;

        // Attach color texture and depth texture to FBO
        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id))
            TraceLog(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully with depth texture", target.id);
        else
            TraceLog(LOG_WARNING, "FBO: Framebuffer object is not complete");

        rlDisableFramebuffer();
    }
    else
        TraceLog(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

void PostProcessingRenderer::UnloadRenderTextureWithDepth(RenderTexture2D target)
{
    if (target.id > 0)
    {
        // Color texture attached to FBO is deleted
        rlUnloadTexture(target.texture.id);
        rlUnloadTexture(target.depth.id);

        // NOTE: Depth texture is automatically queried and deleted before deleting framebuffer
        rlUnloadFramebuffer(target.id);
    }
}
