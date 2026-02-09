#include "ShadowRenderer.h"
#include "rlgl.h"

ShadowRenderer::ShadowRenderer()
    : shadowMapWidth(4096),
      shadowMapHeight(4096),
      shadowBias(0.005f),
      enableShadows(true),
      lightDirection({0.3f, 0.5f, 0.8f})
{
}

ShadowRenderer::~ShadowRenderer()
{
    Shutdown();
}

void ShadowRenderer::Init(int width, int height)
{
    shadowMapWidth = width;
    shadowMapHeight = height;

    // Create shadow map render texture with depth attachment
    shadowMap = LoadShadowMap(width, height);

    TraceLog(LOG_INFO, "ShadowRenderer: Shadow map created (%dx%d)", width, height);

    // Load depth rendering shader
    depthShader = LoadShader("assets/shader/shadow.vs", "assets/shader/shadow.fs");

    TraceLog(LOG_INFO, "ShadowRenderer: Depth shader loaded successfully");
}

void ShadowRenderer::Shutdown()
{
    if (shadowMap.id > 0)
        UnloadShadowMap(shadowMap);
    if (depthShader.id > 0)
        UnloadShader(depthShader);

    TraceLog(LOG_INFO, "ShadowRenderer: Shutdown complete");
}

void ShadowRenderer::BeginShadowPass(Vector3 lightPos, Vector3 lightTarget, Vector3 lightUp)
{
    if (!enableShadows)
        return;

    // Calculate light space matrix
    lightSpaceMatrix = CalculateLightSpaceMatrix(lightPos, lightTarget, lightUp);

    // Begin rendering to shadow map
    BeginTextureMode(shadowMap);
    ClearBackground(WHITE); // Clear to white (far distance)
}

void ShadowRenderer::EndShadowPass()
{
    if (!enableShadows)
        return;

    EndTextureMode();
}

RenderTexture2D ShadowRenderer::LoadShadowMap(int width, int height)
{
    RenderTexture2D target = {0};

    target.id = rlLoadFramebuffer();

    if (target.id > 0)
    {
        rlEnableFramebuffer(target.id);

        // Create a dummy color texture attachment (required for rendering to work)
        target.texture.id = rlLoadTexture(0, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        target.texture.width = width;
        target.texture.height = height;
        target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        target.texture.mipmaps = 1;

        // Create depth texture for shadow map
        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19; // DEPTH_COMPONENT_24BIT
        target.depth.mipmaps = 1;

        // Attach both color and depth textures to FBO
        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if FBO is complete
        if (rlFramebufferComplete(target.id))
            TraceLog(LOG_INFO, "ShadowRenderer: Shadow map FBO created successfully [ID %i]", target.id);
        else
            TraceLog(LOG_WARNING, "ShadowRenderer: Shadow map FBO is not complete");

        rlDisableFramebuffer();
    }
    else
        TraceLog(LOG_WARNING, "ShadowRenderer: Failed to create shadow map FBO");

    return target;
}

void ShadowRenderer::UnloadShadowMap(RenderTexture2D &target)
{
    if (target.id > 0)
    {
        rlUnloadTexture(target.texture.id);
        rlUnloadTexture(target.depth.id);
        rlUnloadFramebuffer(target.id);
    }
}

Matrix ShadowRenderer::CalculateLightSpaceMatrix(Vector3 lightPos, Vector3 lightTarget, Vector3 lightUp)
{
    // Create view matrix (light looking towards target)
    Matrix view = MatrixLookAt(lightPos, lightTarget, lightUp);

    // Create orthographic projection matrix for directional light
    float orthoSize = 100.0f;
    Matrix projection = MatrixOrtho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 500.0f);

    // Combine into light space matrix (projection * view order)
    Matrix lightSpaceMatrix = MatrixMultiply(projection, view);

    return lightSpaceMatrix;
}

void ShadowRenderer::DebugDrawShadowMap(int x, int y, int width, int height)
{
    if (!enableShadows || shadowMap.depth.id == 0)
        return;

    // Draw the shadow map depth texture for debugging
    Rectangle sourceRec = {0, 0, (float)shadowMap.depth.width, (float)-shadowMap.depth.height};
    Rectangle destRec = {(float)x, (float)y, (float)width, (float)height};
    DrawTexturePro(shadowMap.depth, sourceRec, destRec, {0, 0}, 0.0f, WHITE);
    DrawRectangleLines(x, y, width, height, GREEN);
}
