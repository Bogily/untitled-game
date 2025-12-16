#include "Renderer.h"
#include "raymath.h"
#include <cstdio>

Renderer::Renderer()
    : screenWidth(0), screenHeight(0), quadVAO(0), quadVBO(0), fogEnabled(true), fogDistance(10.0f), fogDensity(0.15f)
{
    sceneTexture = {0};
    compositeShader = {0};
    debugShader = {0};
}

Renderer::~Renderer()
{
    Shutdown();
}

void Renderer::Init(int width, int height)
{
    screenWidth = width;
    screenHeight = height;

    TraceLog(LOG_INFO, "Initializing Renderer (%dx%d)", width, height);

    CreateSceneTexture();
    CreateFullscreenQuad();
    LoadShaders();

    TraceLog(LOG_INFO, "Renderer initialized successfully");
}

void Renderer::Shutdown()
{
    // Unload scene texture
    if (sceneTexture.id > 0)
    {
        UnloadRenderTexture(sceneTexture);
        sceneTexture.id = 0;
    }

    // Delete quad
    if (quadVAO)
    {
        glDeleteVertexArrays(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);
        quadVAO = 0;
    }

    // Unload shaders
    if (compositeShader.id > 0)
        UnloadShader(compositeShader);
    if (debugShader.id > 0)
        UnloadShader(debugShader);
}

void Renderer::CreateSceneTexture()
{
    // Use raylib's RenderTexture which includes both color and depth
    sceneTexture = LoadRenderTexture(screenWidth, screenHeight);

    // We need to replace the depth renderbuffer with a readable depth texture
    glBindFramebuffer(GL_FRAMEBUFFER, sceneTexture.id);

    // Delete raylib's depth renderbuffer if it exists
    if (sceneTexture.depth.id > 0)
    {
        glDeleteRenderbuffers(1, &sceneTexture.depth.id);
    }

    // Create a depth texture instead of renderbuffer
    unsigned int depthTexture;
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, screenWidth, screenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach depth texture to FBO (replaces the renderbuffer)
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    // Store depth texture ID in the depth component
    sceneTexture.depth.id = depthTexture;

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        TraceLog(LOG_ERROR, "Scene framebuffer is not complete!");
    }
    else
    {
        TraceLog(LOG_INFO, "Scene texture with readable depth created successfully (%dx%d)", screenWidth, screenHeight);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::CreateFullscreenQuad()
{
    float quadVertices[] = {
        // Positions        // TexCoords
        -1.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        -1.0f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        -1.0f,
        0.0f,
        1.0f,
        1.0f,
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);

    // TexCoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void Renderer::LoadShaders()
{
    // Composite shader (combines scene with fog)
    compositeShader = LoadShader("assets/shader/composite.vs", "assets/shader/composite.fs");

    // Debug shader for visualizing buffers
    debugShader = LoadShader("assets/shader/debug.vs", "assets/shader/debug.fs");

    TraceLog(LOG_INFO, "Shaders loaded");
}

void Renderer::RenderQuad()
{
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void Renderer::BeginSceneCapture()
{
    BeginTextureMode(sceneTexture);
    ClearBackground(RAYWHITE);
}

void Renderer::EndSceneCapture()
{
    EndTextureMode();
}

void Renderer::ApplyFogAndRender(Camera3D &camera)
{
    // === COMPOSITE PASS WITH FOG ===
    glDisable(GL_DEPTH_TEST);

    BeginShaderMode(compositeShader);

    // Bind scene color texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture.texture.id);
    SetShaderValue(compositeShader, GetShaderLocation(compositeShader, "sceneTexture"), (int[]){0}, SHADER_UNIFORM_INT);

    // Bind depth texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sceneTexture.depth.id);
    SetShaderValue(compositeShader, GetShaderLocation(compositeShader, "depthTexture"), (int[]){1}, SHADER_UNIFORM_INT);

    // Set fog parameters
    int fogOn = fogEnabled ? 1 : 0;
    SetShaderValue(compositeShader, GetShaderLocation(compositeShader, "fogEnabled"), &fogOn, SHADER_UNIFORM_INT);
    SetShaderValue(compositeShader, GetShaderLocation(compositeShader, "fogDistance"), &fogDistance, SHADER_UNIFORM_FLOAT);
    SetShaderValue(compositeShader, GetShaderLocation(compositeShader, "fogDensity"), &fogDensity, SHADER_UNIFORM_FLOAT);

    // Set camera parameters for depth reconstruction
    float aspect = (float)screenWidth / (float)screenHeight;
    Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.1f, 1000.0f);
    Matrix invProjection = MatrixInvert(projection);
    SetShaderValueMatrix(compositeShader, GetShaderLocation(compositeShader, "invProjection"), invProjection);

    Vector3 fogColor = {0.8f, 0.8f, 0.8f}; // Light blue-gray fog
    float fogColorArray[3] = {fogColor.x, fogColor.y, fogColor.z};
    SetShaderValue(compositeShader, GetShaderLocation(compositeShader, "fogColor"), fogColorArray, SHADER_UNIFORM_VEC3);

    RenderQuad();

    EndShaderMode();
    glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawDebugBuffer(int bufferIndex, Camera3D &camera)
{
    glDisable(GL_DEPTH_TEST);

    BeginShaderMode(debugShader);

    glActiveTexture(GL_TEXTURE0);

    int mode = 0; // 0 = color, 1 = depth
    switch (bufferIndex)
    {
    case 0:
        glBindTexture(GL_TEXTURE_2D, sceneTexture.texture.id);
        mode = 0;
        break;
    case 1:
        glBindTexture(GL_TEXTURE_2D, sceneTexture.depth.id);
        mode = 1;
        break;
    case 2: // Normals view - uses depth texture
        glBindTexture(GL_TEXTURE_2D, sceneTexture.depth.id);
        mode = 3;
        break;
    default:
        glBindTexture(GL_TEXTURE_2D, sceneTexture.texture.id);
        mode = 0;
        break;
    }

    SetShaderValue(debugShader, GetShaderLocation(debugShader, "texture0"), (int[]){0}, SHADER_UNIFORM_INT);
    SetShaderValue(debugShader, GetShaderLocation(debugShader, "mode"), &mode, SHADER_UNIFORM_INT);

    // For normal view, we need additional uniforms
    if (mode == 3)
    {
        // Bind depth texture to texture unit 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, sceneTexture.depth.id);
        SetShaderValue(debugShader, GetShaderLocation(debugShader, "depthTexture"), (int[]){1}, SHADER_UNIFORM_INT);

        // Set inverse projection matrix
        float aspect = (float)screenWidth / (float)screenHeight;
        Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.1f, 1000.0f);
        Matrix invProjection = MatrixInvert(projection);
        SetShaderValueMatrix(debugShader, GetShaderLocation(debugShader, "invProjection"), invProjection);

        // Set screen size
        float screenSizeVec[2] = {(float)screenWidth, (float)screenHeight};
        SetShaderValue(debugShader, GetShaderLocation(debugShader, "screenSize"), screenSizeVec, SHADER_UNIFORM_VEC2);
    }

    RenderQuad();

    EndShaderMode();
    glEnable(GL_DEPTH_TEST);
}