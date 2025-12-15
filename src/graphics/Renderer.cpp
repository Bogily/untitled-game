#include "Renderer.h"
#include "raymath.h"
#include <cstdio>

Renderer::Renderer()
    : screenWidth(0), screenHeight(0), ssaoFBO(0), ssaoColorBuffer(0), ssaoBlurFBO(0), ssaoBlurColorBuffer(0), noiseTexture(0), quadVAO(0), quadVBO(0), ssaoEnabled(true), ssaoRadius(0.5f), ssaoBias(0.025f), ssaoIntensity(1.5f), ssaoKernelSize(32)
{
    sceneTexture = {0};
    ssaoShader = {0};
    ssaoBlurShader = {0};
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

    TraceLog(LOG_INFO, "Initializing SSAO Renderer (%dx%d)", width, height);

    CreateSceneTexture();
    CreateSSAOBuffers();
    CreateFullscreenQuad();
    GenerateSSAOKernel();
    GenerateNoiseTexture();
    LoadShaders();

    TraceLog(LOG_INFO, "SSAO Renderer initialized successfully");
}

void Renderer::Shutdown()
{
    // Unload scene texture
    if (sceneTexture.id > 0)
    {
        UnloadRenderTexture(sceneTexture);
        sceneTexture.id = 0;
    }

    // Delete SSAO buffers
    if (ssaoFBO)
    {
        glDeleteFramebuffers(1, &ssaoFBO);
        glDeleteTextures(1, &ssaoColorBuffer);
        glDeleteFramebuffers(1, &ssaoBlurFBO);
        glDeleteTextures(1, &ssaoBlurColorBuffer);
        glDeleteTextures(1, &noiseTexture);
        ssaoFBO = 0;
    }

    // Delete quad
    if (quadVAO)
    {
        glDeleteVertexArrays(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);
        quadVAO = 0;
    }

    // Unload shaders
    if (ssaoShader.id > 0)
        UnloadShader(ssaoShader);
    if (ssaoBlurShader.id > 0)
        UnloadShader(ssaoBlurShader);
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

void Renderer::CreateSSAOBuffers()
{
    // SSAO color buffer
    glGenFramebuffers(1, &ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        TraceLog(LOG_ERROR, "SSAO framebuffer is not complete!");
    }

    // SSAO blur buffer
    glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);

    glGenTextures(1, &ssaoBlurColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurColorBuffer, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        TraceLog(LOG_ERROR, "SSAO blur framebuffer is not complete!");
    }

    TraceLog(LOG_INFO, "SSAO buffers created successfully (%dx%d)", screenWidth, screenHeight);

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
        1.0f,
        -1.0f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        0.0f,
        1.0f,
        1.0f,
        1.0f,
        -1.0f,
        0.0f,
        1.0f,
        0.0f,
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

void Renderer::GenerateSSAOKernel()
{
    std::random_device rd;
    std::mt19937 gen(42); // Fixed seed for consistent results
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);

    ssaoKernel.clear();
    ssaoKernel.reserve(ssaoKernelSize);

    for (int i = 0; i < ssaoKernelSize; ++i)
    {
        // Generate random sample in hemisphere
        Vector3 sample = {
            randomFloats(gen) * 2.0f - 1.0f, // -1 to 1
            randomFloats(gen) * 2.0f - 1.0f, // -1 to 1
            randomFloats(gen)                // 0 to 1 (hemisphere)
        };

        // Normalize
        float len = sqrtf(sample.x * sample.x + sample.y * sample.y + sample.z * sample.z);
        if (len > 0.0001f)
        {
            sample.x /= len;
            sample.y /= len;
            sample.z /= len;
        }

        // Scale by random value
        float randomScale = randomFloats(gen);
        sample.x *= randomScale;
        sample.y *= randomScale;
        sample.z *= randomScale;

        // Scale samples to focus more samples closer to the fragment
        float scale = (float)i / (float)ssaoKernelSize;
        scale = Lerp(0.1f, 1.0f, scale * scale);
        sample.x *= scale;
        sample.y *= scale;
        sample.z *= scale;

        ssaoKernel.push_back(sample);
    }

    TraceLog(LOG_INFO, "Generated SSAO kernel with %d samples", ssaoKernelSize);
}

void Renderer::GenerateNoiseTexture()
{
    std::random_device rd;
    std::mt19937 gen(123); // Fixed seed
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);

    // 4x4 noise texture for rotating the sample kernel
    std::vector<float> ssaoNoise;
    ssaoNoise.reserve(16 * 3);
    for (int i = 0; i < 16; i++)
    {
        // Rotate around z-axis (in tangent space)
        ssaoNoise.push_back(randomFloats(gen) * 2.0f - 1.0f); // x
        ssaoNoise.push_back(randomFloats(gen) * 2.0f - 1.0f); // y
        ssaoNoise.push_back(0.0f);                            // z
    }

    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, ssaoNoise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    TraceLog(LOG_INFO, "Generated SSAO noise texture (4x4)");
}

void Renderer::LoadShaders()
{
    // SSAO shader (depth-based)
    ssaoShader = LoadShader("assets/shader/ssao_depth.vs", "assets/shader/ssao_depth.fs");

    // SSAO blur shader
    ssaoBlurShader = LoadShader("assets/shader/ssao_blur.vs", "assets/shader/ssao_blur.fs");

    // Composite shader (combines scene with SSAO)
    compositeShader = LoadShader("assets/shader/composite.vs", "assets/shader/composite.fs");

    // Debug shader for visualizing buffers
    debugShader = LoadShader("assets/shader/debug.vs", "assets/shader/debug.fs");

    TraceLog(LOG_INFO, "SSAO shaders loaded");
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

void Renderer::ApplySSAOAndRender(Camera3D &camera)
{
    // === SSAO PASS ===
    if (ssaoEnabled)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        BeginShaderMode(ssaoShader);

        // Bind depth texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTexture.depth.id);
        SetShaderValue(ssaoShader, GetShaderLocation(ssaoShader, "depthTexture"), (int[]){0}, SHADER_UNIFORM_INT);

        // Bind noise texture
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        SetShaderValue(ssaoShader, GetShaderLocation(ssaoShader, "noiseTexture"), (int[]){1}, SHADER_UNIFORM_INT);

        // Set projection matrix
        float aspect = (float)screenWidth / (float)screenHeight;
        Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.1f, 1000.0f);
        Matrix invProjection = MatrixInvert(projection);
        SetShaderValueMatrix(ssaoShader, GetShaderLocation(ssaoShader, "projection"), projection);
        SetShaderValueMatrix(ssaoShader, GetShaderLocation(ssaoShader, "invProjection"), invProjection);

        // Set kernel samples
        for (int i = 0; i < ssaoKernelSize; i++)
        {
            char uniformName[32];
            snprintf(uniformName, sizeof(uniformName), "samples[%d]", i);
            float sample[3] = {ssaoKernel[i].x, ssaoKernel[i].y, ssaoKernel[i].z};
            SetShaderValue(ssaoShader, GetShaderLocation(ssaoShader, uniformName), sample, SHADER_UNIFORM_VEC3);
        }

        // Set parameters
        float noiseScale[2] = {(float)screenWidth / 4.0f, (float)screenHeight / 4.0f};
        SetShaderValue(ssaoShader, GetShaderLocation(ssaoShader, "noiseScale"), noiseScale, SHADER_UNIFORM_VEC2);
        SetShaderValue(ssaoShader, GetShaderLocation(ssaoShader, "kernelSize"), &ssaoKernelSize, SHADER_UNIFORM_INT);
        SetShaderValue(ssaoShader, GetShaderLocation(ssaoShader, "radius"), &ssaoRadius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(ssaoShader, GetShaderLocation(ssaoShader, "bias"), &ssaoBias, SHADER_UNIFORM_FLOAT);

        float screenSize[2] = {(float)screenWidth, (float)screenHeight};
        SetShaderValue(ssaoShader, GetShaderLocation(ssaoShader, "screenSize"), screenSize, SHADER_UNIFORM_VEC2);

        RenderQuad();

        EndShaderMode();

        // === BLUR PASS ===
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
        glClear(GL_COLOR_BUFFER_BIT);

        BeginShaderMode(ssaoBlurShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
        SetShaderValue(ssaoBlurShader, GetShaderLocation(ssaoBlurShader, "ssaoInput"), (int[]){0}, SHADER_UNIFORM_INT);

        RenderQuad();

        EndShaderMode();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // === COMPOSITE PASS ===
    glDisable(GL_DEPTH_TEST);

    BeginShaderMode(compositeShader);

    // Bind scene color texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture.texture.id);
    SetShaderValue(compositeShader, GetShaderLocation(compositeShader, "sceneTexture"), (int[]){0}, SHADER_UNIFORM_INT);

    // Bind SSAO texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ssaoEnabled ? ssaoBlurColorBuffer : 0);
    SetShaderValue(compositeShader, GetShaderLocation(compositeShader, "ssaoTexture"), (int[]){1}, SHADER_UNIFORM_INT);

    int ssaoOn = ssaoEnabled ? 1 : 0;
    SetShaderValue(compositeShader, GetShaderLocation(compositeShader, "ssaoEnabled"), &ssaoOn, SHADER_UNIFORM_INT);
    SetShaderValue(compositeShader, GetShaderLocation(compositeShader, "ssaoIntensity"), &ssaoIntensity, SHADER_UNIFORM_FLOAT);

    RenderQuad();

    EndShaderMode();
    glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawDebugBuffer(int bufferIndex, Camera3D &camera)
{
    glDisable(GL_DEPTH_TEST);

    BeginShaderMode(debugShader);

    glActiveTexture(GL_TEXTURE0);

    int mode = 0; // 0 = color, 1 = depth, 2 = ssao, 3 = normals
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
    case 2:
        glBindTexture(GL_TEXTURE_2D, ssaoBlurColorBuffer);
        mode = 2;
        break;
    case 3: // Normals view - uses depth texture
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
