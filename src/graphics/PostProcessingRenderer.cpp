#include "PostProcessingRenderer.h"
#include "rlgl.h"
#include <glad/glad.h>

PostProcessingRenderer::PostProcessingRenderer()
    : width(0), height(0),
      enableGrayscale(false),
      enableDepthDebug(false),
      msaaTexture({0})
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
    // Clean up MSAA textures if enabled
    if (IsMSAAEnabled())
        DisableMSAA();

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
    if (IsMSAAEnabled())
    {
        // Render to MSAA framebuffer
        rlEnableFramebuffer(msaaTexture.fboMS);
    }
    else
    {
        // Render to regular texture mode
        BeginTextureMode(sceneTexture);
    }
}

void PostProcessingRenderer::EndSceneCapture()
{
    if (IsMSAAEnabled())
    {
        // End MSAA rendering and perform resolve
        rlDisableFramebuffer();
        ResolveMSAA(msaaTexture);
    }
    else
    {
        EndTextureMode();
    }
}

void PostProcessingRenderer::ApplyEffects()
{
    // Choose which texture to use based on MSAA status
    RenderTexture2D targetTexture = sceneTexture;
    if (IsMSAAEnabled())
    {
        // Create a temporary RenderTexture2D structure using resolved MSAA texture
        targetTexture.texture = msaaTexture.colorResolved;
        targetTexture.depth = msaaTexture.depthResolved;
    }

    if (enableDepthDebug)
    {
        // Render depth buffer with shader
        BeginShaderMode(depthShader);
        int depthLoc = GetShaderLocation(depthShader, "depthTexture");
        int flipTextureLoc = GetShaderLocation(depthShader, "flipY");
        SetShaderValue(depthShader, flipTextureLoc, (int[]){1}, SHADER_UNIFORM_INT);
        SetShaderValueTexture(depthShader, depthLoc, targetTexture.depth);

        // Render fullscreen quad with depth texture
        Rectangle sourceRec = {0, 0, (float)targetTexture.depth.width, (float)-targetTexture.depth.height};
        Rectangle destRec = {0, 0, (float)width, (float)height};
        DrawTexturePro(targetTexture.depth, sourceRec, destRec, {0, 0}, 0.0f, WHITE);

        EndShaderMode();
    }
    else if (enableGrayscale)
    {
        RenderFullscreenQuad(grayscaleShader, targetTexture);
    }
    else
    {
        // Just render the scene texture directly
        Rectangle sourceRec = {0, 0, (float)targetTexture.texture.width, (float)-targetTexture.texture.height};
        Rectangle destRec = {0, 0, (float)width, (float)height};
        DrawTexturePro(targetTexture.texture, sourceRec, destRec, {0, 0}, 0.0f, WHITE);
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

// MSAA Implementation

unsigned int PostProcessingRenderer::QueryMaxMSAASamples()
{
    GLint maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

    // Clamp to supported levels: 4, 8, or 16
    if (maxSamples >= 16)
        return 16;
    else if (maxSamples >= 8)
        return 8;
    else if (maxSamples >= 4)
        return 4;
    else
        return 0; // MSAA not supported
}

MSAARenderTexture PostProcessingRenderer::LoadMSAARenderTexture(int width, int height, unsigned int sampleCount)
{
    MSAARenderTexture msaaTarget = {0};
    msaaTarget.width = width;
    msaaTarget.height = height;
    msaaTarget.sampleCount = sampleCount;

    if (sampleCount == 0)
    {
        TraceLog(LOG_WARNING, "PostProcessingRenderer: MSAA sample count is 0, returning empty texture");
        return msaaTarget;
    }

    // Create MSAA framebuffer for rendering
    msaaTarget.fboMS = rlLoadFramebuffer();

    if (msaaTarget.fboMS > 0)
    {
        rlEnableFramebuffer(msaaTarget.fboMS);

        // Create multisampled color texture (GL_TEXTURE_2D_MULTISAMPLE)
        glGenTextures(1, &msaaTarget.colorMS.id);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaTarget.colorMS.id);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, GL_RGBA8, width, height, GL_TRUE);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

        msaaTarget.colorMS.width = width;
        msaaTarget.colorMS.height = height;
        msaaTarget.colorMS.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        msaaTarget.colorMS.mipmaps = 1;

        // Create multisampled depth texture (GL_TEXTURE_2D_MULTISAMPLE)
        glGenTextures(1, &msaaTarget.depthMS.id);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaTarget.depthMS.id);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, GL_DEPTH_COMPONENT24, width, height, GL_TRUE);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

        msaaTarget.depthMS.width = width;
        msaaTarget.depthMS.height = height;
        msaaTarget.depthMS.format = 19; // DEPTH_COMPONENT_24BIT
        msaaTarget.depthMS.mipmaps = 1;

        // Attach MSAA textures to MSAA FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msaaTarget.colorMS.id, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, msaaTarget.depthMS.id, 0);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status == GL_FRAMEBUFFER_COMPLETE)
        {
            TraceLog(LOG_INFO, "PostProcessingRenderer: MSAA framebuffer created successfully (%dX MSAA, %dx%d)", sampleCount, width, height);
        }
        else
        {
            TraceLog(LOG_WARNING, "PostProcessingRenderer: MSAA framebuffer is not complete (status: 0x%x)", status);
        }

        rlDisableFramebuffer();
    }
    else
    {
        TraceLog(LOG_WARNING, "PostProcessingRenderer: Failed to create MSAA framebuffer");
        return msaaTarget;
    }

    // Create resolve framebuffer for blit operation
    msaaTarget.fboResolve = rlLoadFramebuffer();

    if (msaaTarget.fboResolve > 0)
    {
        rlEnableFramebuffer(msaaTarget.fboResolve);

        // Create resolved color texture (regular 2D texture)
        msaaTarget.colorResolved.id = rlLoadTexture(0, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        msaaTarget.colorResolved.width = width;
        msaaTarget.colorResolved.height = height;
        msaaTarget.colorResolved.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        msaaTarget.colorResolved.mipmaps = 1;

        // Create resolved depth texture
        msaaTarget.depthResolved.id = rlLoadTextureDepth(width, height, false);
        msaaTarget.depthResolved.width = width;
        msaaTarget.depthResolved.height = height;
        msaaTarget.depthResolved.format = 19;
        msaaTarget.depthResolved.mipmaps = 1;

        // Attach resolved textures to resolve FBO
        rlFramebufferAttach(msaaTarget.fboResolve, msaaTarget.colorResolved.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(msaaTarget.fboResolve, msaaTarget.depthResolved.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        if (rlFramebufferComplete(msaaTarget.fboResolve))
        {
            TraceLog(LOG_INFO, "PostProcessingRenderer: Resolve framebuffer created successfully");
        }
        else
        {
            TraceLog(LOG_WARNING, "PostProcessingRenderer: Resolve framebuffer is not complete");
        }

        rlDisableFramebuffer();
    }
    else
    {
        TraceLog(LOG_WARNING, "PostProcessingRenderer: Failed to create resolve framebuffer");
    }

    return msaaTarget;
}

void PostProcessingRenderer::ResolveMSAA(const MSAARenderTexture &msaaTexture)
{
    if (msaaTexture.fboMS == 0 || msaaTexture.fboResolve == 0)
    {
        TraceLog(LOG_WARNING, "PostProcessingRenderer: Cannot resolve MSAA - framebuffers not initialized");
        return;
    }

    // Bind MSAA FBO as read source
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaTexture.fboMS);

    // Bind resolve FBO as draw target
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, msaaTexture.fboResolve);

    // Blit color buffer
    glBlitFramebuffer(0, 0, msaaTexture.width, msaaTexture.height,
                      0, 0, msaaTexture.width, msaaTexture.height,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // Blit depth buffer
    glBlitFramebuffer(0, 0, msaaTexture.width, msaaTexture.height,
                      0, 0, msaaTexture.width, msaaTexture.height,
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    // Reset framebuffer binding
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessingRenderer::UnloadMSAARenderTexture(MSAARenderTexture &msaaTexture)
{
    if (msaaTexture.fboMS > 0)
    {
        if (msaaTexture.colorMS.id > 0)
        {
            glDeleteTextures(1, &msaaTexture.colorMS.id);
            msaaTexture.colorMS.id = 0;
        }

        if (msaaTexture.depthMS.id > 0)
        {
            glDeleteTextures(1, &msaaTexture.depthMS.id);
            msaaTexture.depthMS.id = 0;
        }

        rlUnloadFramebuffer(msaaTexture.fboMS);
        msaaTexture.fboMS = 0;
    }

    if (msaaTexture.fboResolve > 0)
    {
        if (msaaTexture.colorResolved.id > 0)
        {
            rlUnloadTexture(msaaTexture.colorResolved.id);
            msaaTexture.colorResolved.id = 0;
        }

        if (msaaTexture.depthResolved.id > 0)
        {
            rlUnloadTexture(msaaTexture.depthResolved.id);
            msaaTexture.depthResolved.id = 0;
        }

        rlUnloadFramebuffer(msaaTexture.fboResolve);
        msaaTexture.fboResolve = 0;
    }

    msaaTexture.sampleCount = 0;
}

void PostProcessingRenderer::EnableMSAA(unsigned int sampleCount)
{
    if (sampleCount == 0)
    {
        TraceLog(LOG_WARNING, "PostProcessingRenderer: Cannot enable MSAA with 0 samples");
        return;
    }

    // Disable existing MSAA if any
    if (IsMSAAEnabled())
        DisableMSAA();

    // Validate sample count
    if (sampleCount != 4 && sampleCount != 8 && sampleCount != 16)
    {
        TraceLog(LOG_WARNING, "PostProcessingRenderer: Invalid MSAA sample count %u, must be 4, 8, or 16", sampleCount);
        return;
    }

    // Create MSAA render texture
    msaaTexture = LoadMSAARenderTexture(width, height, sampleCount);

    if (IsMSAAEnabled())
    {
        TraceLog(LOG_INFO, "PostProcessingRenderer: MSAA enabled (%uX)", sampleCount);
    }
    else
    {
        TraceLog(LOG_WARNING, "PostProcessingRenderer: Failed to enable MSAA");
    }
}

void PostProcessingRenderer::DisableMSAA()
{
    if (!IsMSAAEnabled())
        return;

    TraceLog(LOG_INFO, "PostProcessingRenderer: MSAA disabled");
    UnloadMSAARenderTexture(msaaTexture);
}
