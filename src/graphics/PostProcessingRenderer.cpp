#include "PostProcessingRenderer.h"
#include "rlgl.h"
#include <glad/glad.h>
#include <cmath>

PostProcessingRenderer::PostProcessingRenderer()
    : width(0), height(0),
      enableGrayscale(false),
      enableDepthDebug(false),
      msaaTexture({0}),
      currentLUTPreset(0),
      lutIntensity(1.0f),
      enableContactShadows(false),
      contactShadowMaxDist(0.1f),
      contactShadowSteps(8),
      contactShadowThickness(0.01f),
      contactShadowIntensity(0.5f),
      enableSSAO(false),
      ssaoNumSamples(8),
      ssaoRadius(0.02f),
      ssaoBias(0.001f),
      ssaoIntensity(0.5f),
      ssaoContrast(1.0f)
{
    lutTexture.id = 0;
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

    // Create ping-pong textures for effect chaining (no depth needed)
    pingPongTextures[0] = LoadRenderTexture(width, height);
    pingPongTextures[1] = LoadRenderTexture(width, height);

    TraceLog(LOG_INFO, "PostProcessingRenderer: Render texture created with depth (%dx%d)", width, height);

    // Load shaders
    grayscaleShader = LoadShader(0, "assets/shader/grayscale.fs");
    depthShader = LoadShader(0, "assets/shader/depth_render.fs");
    colorGradingShader = LoadShader(0, "assets/shader/color_grading.fs");
    screenSpaceShadowsShader = LoadShader(0, "assets/shader/screen_space_shadows.fs");
    ssaoShader = LoadShader(0, "assets/shader/ssao.fs");

    TraceLog(LOG_INFO, "PostProcessingRenderer: Grayscale shader loaded successfully");
    TraceLog(LOG_INFO, "PostProcessingRenderer: Depth render shader loaded successfully");
    TraceLog(LOG_INFO, "PostProcessingRenderer: Color grading shader loaded successfully");
    TraceLog(LOG_INFO, "PostProcessingRenderer: Screen-space shadows shader loaded successfully");
    TraceLog(LOG_INFO, "PostProcessingRenderer: SSAO shader loaded successfully");

    // Generate identity LUT by default (no color change)
    lutTexture = Generate3DLUT(0);
    TraceLog(LOG_INFO, "PostProcessingRenderer: Default LUT generated");
}

void PostProcessingRenderer::Shutdown()
{
    // Clean up MSAA textures if enabled
    if (IsMSAAEnabled())
        DisableMSAA();

    if (sceneTexture.id > 0)
        UnloadRenderTextureWithDepth(sceneTexture);
    if (pingPongTextures[0].id > 0)
        UnloadRenderTexture(pingPongTextures[0]);
    if (pingPongTextures[1].id > 0)
        UnloadRenderTexture(pingPongTextures[1]);
    if (grayscaleShader.id > 0)
        UnloadShader(grayscaleShader);
    if (depthShader.id > 0)
        UnloadShader(depthShader);
    if (colorGradingShader.id > 0)
        UnloadShader(colorGradingShader);
    if (screenSpaceShadowsShader.id > 0)
        UnloadShader(screenSpaceShadowsShader);
    if (ssaoShader.id > 0)
        UnloadShader(ssaoShader);
    if (lutTexture.id > 0)
        UnloadTexture(lutTexture);

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

    // Ping-pong rendering: apply effects in sequence
    int currentRead = 0;  // Index of texture to read from
    int currentWrite = 1; // Index of texture to write to
    bool hasEffects = false;

    // Helper lambda to draw fullscreen quad
    auto drawFullscreen = [this](Texture2D tex)
    {
        Rectangle sourceRec = {0, 0, (float)tex.width, (float)-tex.height};
        Rectangle destRec = {0, 0, (float)width, (float)height};
        DrawTexturePro(tex, sourceRec, destRec, {0, 0}, 0.0f, WHITE);
    };

    // Helper lambda to apply shader pass
    auto applyShaderPass = [&](Shader shader, Texture2D source, auto setupShader)
    {
        ScopedTextureMode texMode(pingPongTextures[currentWrite]);
        ScopedShaderMode shaderMode(shader);
        setupShader();
        drawFullscreen(source);
        hasEffects = true;
        currentRead = currentWrite;
        currentWrite = 1 - currentWrite;
    };

    // Special case: depth debug renders directly and skips other effects
    if (enableDepthDebug)
    {
        ScopedShaderMode shaderMode(depthShader);
        ShaderUtil util(depthShader);
        util.SetTexture("depthTexture", targetTexture.depth);
        drawFullscreen(targetTexture.depth);
        return; // Skip all other effects
    }

    // First effect: grayscale or color grading
    if (enableGrayscale)
    {
        applyShaderPass(grayscaleShader, targetTexture.texture, []() {});
    }
    else if (currentLUTPreset > 0 && lutIntensity > 0.0f)
    {
        applyShaderPass(colorGradingShader, targetTexture.texture, [&]()
                        {
            ShaderUtil util(colorGradingShader);
            util.SetTexture("lutTexture", lutTexture);
            util.SetFloat("lutIntensity", lutIntensity); });
    }

    // Apply contact shadows if enabled
    if (enableContactShadows && contactShadowIntensity > 0.0f)
    {
        Texture2D sourceTexture = hasEffects ? pingPongTextures[currentRead].texture : targetTexture.texture;

        applyShaderPass(screenSpaceShadowsShader, sourceTexture, [&]()
                        {
            ShaderUtil util(screenSpaceShadowsShader);
            util.SetTexture("depthTexture", targetTexture.depth);
            util.SetFloat("maxDistance", contactShadowMaxDist);
            util.SetInt("numSteps", contactShadowSteps);
            util.SetFloat("thickness", contactShadowThickness);
            util.SetFloat("intensity", contactShadowIntensity);
            util.SetInt("enabled", 1); });
    }

    // Apply SSAO if enabled
    if (enableSSAO && ssaoIntensity > 0.0f)
    {
        Texture2D sourceTexture = hasEffects ? pingPongTextures[currentRead].texture : targetTexture.texture;

        applyShaderPass(ssaoShader, sourceTexture, [&]()
                        {
            ShaderUtil util(ssaoShader);
            util.SetTexture("depthTexture", targetTexture.depth);
            util.SetInt("numSamples", ssaoNumSamples);
            util.SetFloat("radius", ssaoRadius);
            util.SetFloat("bias", ssaoBias);
            util.SetFloat("intensity", ssaoIntensity);
            util.SetFloat("contrast", ssaoContrast);
            util.SetInt("enabled", 1); });
    }

    // Final render to screen
    if (hasEffects)
    {
        drawFullscreen(pingPongTextures[currentRead].texture);
    }
    else
    {
        drawFullscreen(targetTexture.texture);
    }
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

// Color Grading Implementation

void PostProcessingRenderer::SetColorGradingPreset(int preset)
{
    if (preset == currentLUTPreset)
        return;

    currentLUTPreset = preset;

    // Unload old LUT
    if (lutTexture.id > 0)
        UnloadTexture(lutTexture);

    // Generate new LUT
    lutTexture = Generate3DLUT(preset);

    const char *presetNames[] = {"None", "Warm", "Cool", "Cinematic", "Vintage", "Noir"};
    if (preset >= 0 && preset < 6)
        TraceLog(LOG_INFO, "PostProcessingRenderer: Color grading preset set to %s", presetNames[preset]);
}

Texture PostProcessingRenderer::Generate3DLUT(int preset)
{
    const int LUT_SIZE = 32;                                  // 32x32x32 LUT
    const int totalSize = LUT_SIZE * LUT_SIZE * LUT_SIZE * 3; // RGB

    // Create identity LUT
    unsigned char *lutData = CreateIdentityLUT(LUT_SIZE);

    // Apply preset modifications
    switch (preset)
    {
    case 0: // None (identity)
        break;
    case 1: // Warm
        ApplyWarmGrading(lutData, LUT_SIZE);
        break;
    case 2: // Cool
        ApplyCoolGrading(lutData, LUT_SIZE);
        break;
    case 3: // Cinematic
        ApplyCinematicGrading(lutData, LUT_SIZE);
        break;
    case 4: // Vintage
        ApplyVintageGrading(lutData, LUT_SIZE);
        break;
    case 5: // Noir
        ApplyNoirGrading(lutData, LUT_SIZE);
        break;
    default:
        break;
    }

    // Create 3D texture
    Image lutImage = {
        .data = lutData,
        .width = LUT_SIZE,
        .height = LUT_SIZE * LUT_SIZE,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8};

    Texture texture = LoadTextureFromImage(lutImage);

    // Convert to 3D texture using OpenGL
    glBindTexture(GL_TEXTURE_3D, texture.id);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, LUT_SIZE, LUT_SIZE, LUT_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, lutData);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_3D, 0);

    free(lutData);
    return texture;
}

unsigned char *PostProcessingRenderer::CreateIdentityLUT(int size)
{
    int totalSize = size * size * size * 3;
    unsigned char *data = (unsigned char *)malloc(totalSize);

    for (int b = 0; b < size; b++)
    {
        for (int g = 0; g < size; g++)
        {
            for (int r = 0; r < size; r++)
            {
                int index = (b * size * size + g * size + r) * 3;
                data[index + 0] = (unsigned char)((r * 255) / (size - 1)); // R
                data[index + 1] = (unsigned char)((g * 255) / (size - 1)); // G
                data[index + 2] = (unsigned char)((b * 255) / (size - 1)); // B
            }
        }
    }

    return data;
}

void PostProcessingRenderer::ApplyWarmGrading(unsigned char *data, int size)
{
    int totalPixels = size * size * size;
    for (int i = 0; i < totalPixels; i++)
    {
        int index = i * 3;
        float r = data[index + 0] / 255.0f;
        float g = data[index + 1] / 255.0f;
        float b = data[index + 2] / 255.0f;

        // Add warmth: boost reds/yellows, reduce blues
        r = fminf(r * 1.15f + 0.05f, 1.0f);
        g = fminf(g * 1.05f, 1.0f);
        b = b * 0.85f;

        data[index + 0] = (unsigned char)(r * 255);
        data[index + 1] = (unsigned char)(g * 255);
        data[index + 2] = (unsigned char)(b * 255);
    }
}

void PostProcessingRenderer::ApplyCoolGrading(unsigned char *data, int size)
{
    int totalPixels = size * size * size;
    for (int i = 0; i < totalPixels; i++)
    {
        int index = i * 3;
        float r = data[index + 0] / 255.0f;
        float g = data[index + 1] / 255.0f;
        float b = data[index + 2] / 255.0f;

        // Add coolness: boost blues, reduce reds
        r = r * 0.85f;
        g = fminf(g * 1.05f, 1.0f);
        b = fminf(b * 1.2f + 0.05f, 1.0f);

        data[index + 0] = (unsigned char)(r * 255);
        data[index + 1] = (unsigned char)(g * 255);
        data[index + 2] = (unsigned char)(b * 255);
    }
}

void PostProcessingRenderer::ApplyCinematicGrading(unsigned char *data, int size)
{
    int totalPixels = size * size * size;
    for (int i = 0; i < totalPixels; i++)
    {
        int index = i * 3;
        float r = data[index + 0] / 255.0f;
        float g = data[index + 1] / 255.0f;
        float b = data[index + 2] / 255.0f;

        // Cinematic: crushed blacks, lifted shadows, teal/orange look
        float luma = 0.299f * r + 0.587f * g + 0.114f * b;

        // Lift shadows, crush blacks
        if (luma < 0.3f)
            luma = luma * 0.8f + 0.05f;

        // Teal/orange color grading
        r = fminf(r * 1.1f + (luma - r) * 0.1f, 1.0f);
        g = fminf(g * 0.95f, 1.0f);
        b = fminf(b * 1.05f + (luma - b) * 0.15f, 1.0f);

        data[index + 0] = (unsigned char)(r * 255);
        data[index + 1] = (unsigned char)(g * 255);
        data[index + 2] = (unsigned char)(b * 255);
    }
}

void PostProcessingRenderer::ApplyVintageGrading(unsigned char *data, int size)
{
    int totalPixels = size * size * size;
    for (int i = 0; i < totalPixels; i++)
    {
        int index = i * 3;
        float r = data[index + 0] / 255.0f;
        float g = data[index + 1] / 255.0f;
        float b = data[index + 2] / 255.0f;

        // Vintage: sepia tones, reduced saturation, faded look
        float luma = 0.299f * r + 0.587f * g + 0.114f * b;

        // Sepia transformation
        r = fminf(luma * 1.2f, 1.0f);
        g = fminf(luma * 1.0f, 1.0f);
        b = fminf(luma * 0.8f, 1.0f);

        // Fade (lift blacks)
        r = r * 0.85f + 0.15f;
        g = g * 0.85f + 0.15f;
        b = b * 0.85f + 0.15f;

        data[index + 0] = (unsigned char)(r * 255);
        data[index + 1] = (unsigned char)(g * 255);
        data[index + 2] = (unsigned char)(b * 255);
    }
}

void PostProcessingRenderer::ApplyNoirGrading(unsigned char *data, int size)
{
    int totalPixels = size * size * size;
    for (int i = 0; i < totalPixels; i++)
    {
        int index = i * 3;
        float r = data[index + 0] / 255.0f;
        float g = data[index + 1] / 255.0f;
        float b = data[index + 2] / 255.0f;

        // Film noir: high contrast black and white
        float luma = 0.299f * r + 0.587f * g + 0.114f * b;

        // Increase contrast
        luma = (luma - 0.5f) * 1.5f + 0.5f;
        luma = fmaxf(0.0f, fminf(1.0f, luma));

        // Convert to grayscale with high contrast
        data[index + 0] = (unsigned char)(luma * 255);
        data[index + 1] = (unsigned char)(luma * 255);
        data[index + 2] = (unsigned char)(luma * 255);
    }
}

// Screen-Space Shadows Implementation

void PostProcessingRenderer::SetContactShadowParams(float maxDist, int steps, float thickness, float intensity)
{
    // Clamp values to reasonable ranges
    contactShadowMaxDist = fmaxf(0.01f, fminf(0.5f, maxDist));
    contactShadowSteps = fmaxf(4, fminf(64, steps));
    contactShadowThickness = fmaxf(0.001f, fminf(0.1f, thickness));
    contactShadowIntensity = fmaxf(0.0f, fminf(1.0f, intensity));

    TraceLog(LOG_INFO, "PostProcessingRenderer: Contact shadow params updated (dist=%.3f, steps=%d, thickness=%.4f, intensity=%.2f)",
             contactShadowMaxDist, contactShadowSteps, contactShadowThickness, contactShadowIntensity);
}

// Screen-Space Ambient Occlusion Implementation

void PostProcessingRenderer::SetSSAOParams(int samples, float radius, float bias, float intensity, float contrast)
{
    // Clamp values to reasonable ranges
    ssaoNumSamples = fmaxf(4, fminf(32, samples));
    ssaoRadius = fmaxf(0.001f, fminf(0.1f, radius));
    ssaoBias = fmaxf(0.001f, fminf(0.01f, bias));
    ssaoIntensity = fmaxf(0.0f, fminf(2.0f, intensity));
    ssaoContrast = fmaxf(0.5f, fminf(2.0f, contrast));

    TraceLog(LOG_INFO, "PostProcessingRenderer: SSAO params updated (samples=%d, radius=%.4f, bias=%.5f, intensity=%.2f, contrast=%.2f)",
             ssaoNumSamples, ssaoRadius, ssaoBias, ssaoIntensity, ssaoContrast);
}