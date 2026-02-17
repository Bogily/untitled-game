#include "RenderManager.h"
#include "Frustum.h"

RenderManager::RenderManager()
    : screenWidth(0),
      screenHeight(0),
      currentDisplayMode(0),
      initialized(false),
      postProcessingEnabled(true),
      grayscaleEnabled(false),
      depthDebugEnabled(false),
      sunDirection({0.3f, 0.5f, 0.8f}),
      maxSupportedMSAA(MSAALevel::MSAA_NONE)
{
}

RenderManager::~RenderManager()
{
    Shutdown();
}

void RenderManager::Init(int width, int height)
{
    if (initialized)
    {
        TraceLog(LOG_WARNING, "RenderManager: Already initialized");
        return;
    }

    screenWidth = width;
    screenHeight = height;

    // Initialize all rendering subsystems
    TraceLog(LOG_INFO, "RenderManager: Initializing LightRenderer...");
    lightRenderer.Init(width, height);
    TraceLog(LOG_INFO, "RenderManager: Initializing PostProcessingRenderer...");
    postProcessingRenderer.Init(width, height);

    // Query GPU MSAA capabilities
    unsigned int maxMSAASamples = PostProcessingRenderer::QueryMaxMSAASamples();
    if (maxMSAASamples >= 16)
        maxSupportedMSAA = MSAALevel::MSAA_16X;
    else if (maxMSAASamples >= 8)
        maxSupportedMSAA = MSAALevel::MSAA_8X;
    else if (maxMSAASamples >= 4)
        maxSupportedMSAA = MSAALevel::MSAA_4X;
    else
        maxSupportedMSAA = MSAALevel::MSAA_NONE;

    TraceLog(LOG_INFO, "RenderManager: GPU MSAA support detected: max %uX", static_cast<unsigned int>(maxSupportedMSAA));

    TraceLog(LOG_INFO, "RenderManager: Initializing GeometryRenderer...");
    geometryRenderer.Init();
    TraceLog(LOG_INFO, "RenderManager: Initializing ParticleSystem...");
    particleSystem.Init();
    TraceLog(LOG_INFO, "RenderManager: Initializing DebugRenderer...");
    debugRenderer.Init();

    initialized = true;
    TraceLog(LOG_INFO, "RenderManager: Initialized (%dx%d)", width, height);
}

void RenderManager::Shutdown()
{
    if (!initialized)
        return;

    // Shutdown all subsystems
    lightRenderer.Shutdown();
    postProcessingRenderer.Shutdown();
    grassRenderer.Shutdown();
    geometryRenderer.Shutdown();
    particleSystem.Shutdown();
    skyboxRenderer.Unload();
    debugRenderer.Shutdown();
    // WaterRenderer shutdown is handled by its destructor

    initialized = false;
    TraceLog(LOG_INFO, "RenderManager: Shutdown complete");
}

void RenderManager::SetWindowSize(int width, int height)
{
    if (!initialized)
    {
        TraceLog(LOG_WARNING, "RenderManager: Cannot resize - not initialized");
        return;
    }

    if (width == screenWidth && height == screenHeight)
        return;

    screenWidth = width;
    screenHeight = height;

    // Reinitialize rendering subsystems with new dimensions
    TraceLog(LOG_INFO, "RenderManager: Resizing to %dx%d", width, height);

    // NOTE: We do NOT re-init LightRenderer because:
    // 1. It doesn't depend on screen size
    // 2. Re-init would unload the PBR shader, breaking all models currently using it
    // 3. Re-init would clear all lights

    // Store current MSAA level before shutdown
    MSAALevel currentMSAALevel = currentFrameSettings.msaaLevel;

    postProcessingRenderer.Shutdown();
    postProcessingRenderer.Init(width, height);

    // Reapply settings
    postProcessingRenderer.SetGrayscaleEnabled(grayscaleEnabled);

    // Reapply MSAA if it was enabled
    if (currentMSAALevel != MSAALevel::MSAA_NONE)
    {
        postProcessingRenderer.EnableMSAA(GetMSAASampleCount(currentMSAALevel));
    }

    TraceLog(LOG_INFO, "RenderManager: Resize complete");
}

void RenderManager::ApplyDisplayMode(int mode)
{
    currentDisplayMode = mode;

    switch (mode)
    {
    case 0: // Windowed
        ClearWindowState(FLAG_FULLSCREEN_MODE);
        ClearWindowState(FLAG_WINDOW_UNDECORATED);
        ::SetWindowSize(screenWidth, screenHeight);
        ::SetWindowPosition((GetMonitorWidth(0) - screenWidth) / 2, (GetMonitorHeight(0) - screenHeight) / 2);
        TraceLog(LOG_INFO, "RenderManager: Display mode set to Windowed");
        break;

    case 1: // Fullscreen
        SetWindowState(FLAG_FULLSCREEN_MODE);
        ClearWindowState(FLAG_WINDOW_UNDECORATED);
        TraceLog(LOG_INFO, "RenderManager: Display mode set to Fullscreen");
        break;

    case 2: // Borderless windowed
        ClearWindowState(FLAG_FULLSCREEN_MODE);
        SetWindowState(FLAG_WINDOW_UNDECORATED);
        int monitorWidth = GetMonitorWidth(0);
        int monitorHeight = GetMonitorHeight(0);
        ::SetWindowSize(monitorWidth, monitorHeight);
        ::SetWindowPosition(0, 0);
        TraceLog(LOG_INFO, "RenderManager: Display mode set to Borderless (%dx%d)", monitorWidth, monitorHeight);
        break;
    }
}

void RenderManager::BeginFrame()
{
    // BeginDrawing is called by the main game loop before this
    ClearBackground(BLACK);
}

void RenderManager::RenderScene(std::function<void()> sceneRenderer, Camera3D camera)
{
    if (!initialized)
    {
        TraceLog(LOG_ERROR, "RenderManager: Not initialized!");
        return;
    }

    // Choose rendering path based on post-processing state
    if (postProcessingEnabled)
    {
        RenderWithPostProcessing(sceneRenderer);
    }
    else
    {
        RenderDirect(sceneRenderer);
    }
}

void RenderManager::EndFrame()
{
    // EndDrawing is called by the main game loop after this
}

void RenderManager::RenderDirect(std::function<void()> sceneRenderer)
{
    // Simple direct rendering - no post-processing
    sceneRenderer();
}

void RenderManager::RenderWithPostProcessing(std::function<void()> sceneRenderer)
{
    // Render scene to texture
    postProcessingRenderer.BeginSceneCapture();
    ClearBackground(BLACK);
    sceneRenderer();
    postProcessingRenderer.EndSceneCapture();

    // Clear screen before applying post-processing
    ClearBackground(BLACK);

    // Apply post-processing effects
    postProcessingRenderer.ApplyEffects();
}

void RenderManager::EnablePostProcessing(bool enable)
{
    if (postProcessingEnabled == enable)
        return;

    postProcessingEnabled = enable;
    TraceLog(LOG_INFO, "RenderManager: Post-processing %s", enable ? "enabled" : "disabled");
}

void RenderManager::EnableGrayscale(bool enable)
{
    if (grayscaleEnabled == enable)
        return;

    grayscaleEnabled = enable;
    postProcessingRenderer.SetGrayscaleEnabled(enable);
    TraceLog(LOG_INFO, "RenderManager: Grayscale %s", enable ? "enabled" : "disabled");
}

void RenderManager::EnableDepthDebug(bool enable)
{
    if (depthDebugEnabled == enable)
        return;

    depthDebugEnabled = enable;
    postProcessingRenderer.SetDepthDebugEnabled(enable);
    TraceLog(LOG_INFO, "RenderManager: Depth debug %s", enable ? "enabled" : "disabled");
}

void RenderManager::SetSunDirection(Vector3 direction)
{
    sunDirection = direction;
    skyboxRenderer.SetSunDirection(direction);
    waterRenderer.SetLightDirection(direction);
}

void RenderManager::UpdateCamera(Camera3D camera, int maxActiveLights)
{
    lightRenderer.Update(camera, maxActiveLights);
}

void RenderManager::UpdateGrass(float deltaTime, Camera3D camera)
{
    grassRenderer.Update(deltaTime, camera);
}

void RenderManager::UpdateWater(float deltaTime, Camera3D camera)
{
    waterRenderer.Update(deltaTime, camera);
}

void RenderManager::UpdateSkybox(float deltaTime)
{
    skyboxRenderer.Update(deltaTime);
}

void RenderManager::ApplyShaderToModel(Model &model, Vector4 albedo, float metallic, float roughness)
{
    lightRenderer.ApplyToModel(model, albedo, metallic, roughness);
}

void RenderManager::ApplyShaders(const std::vector<ModelMaterial> &models)
{
    for (const auto &modelMat : models)
    {
        lightRenderer.ApplyToModel(*modelMat.model, modelMat.albedo, modelMat.metallic, modelMat.roughness);
    }

    TraceLog(LOG_INFO, "RenderManager: Applied PBR shaders to %d models", (int)models.size());
}

void RenderManager::InitializeSkybox(const char *vertexShader, const char *fragmentShader)
{
    skyboxRenderer.Load(vertexShader, fragmentShader);
}

void RenderManager::ConfigureSkybox(Vector3 skyColor, Vector3 cloudColor, Vector3 sunColor)
{
    skyboxRenderer.SetSkyColor(skyColor);
    skyboxRenderer.SetCloudColor(cloudColor);
    skyboxRenderer.SetSunColor(sunColor);
    skyboxRenderer.SetSunDirection(sunDirection);
}

void RenderManager::InitializeGrass(int bladeCount, float area)
{
    grassRenderer.Init(bladeCount, area);
}

void RenderManager::ConfigureGrass(Vector2 windDirection, float windStrength, float windSpeed)
{
    grassRenderer.SetWindDirection(windDirection);
    grassRenderer.SetWindStrength(windStrength);
    grassRenderer.SetWindSpeed(windSpeed);
}

void RenderManager::InitializeWater(float width, float height, float waterLevel)
{
    waterRenderer.SetWaterSize(width, height);
    waterRenderer.SetWaterLevel(waterLevel);
    waterRenderer.Init();
    waterRenderer.SetLightDirection(sunDirection);
}

void RenderManager::CreateDirectionalLight(Vector3 direction, Vector4 color, float intensity)
{
    lightRenderer.CreateDirectionalLight(direction, color, intensity);
}

void RenderManager::CreatePointLight(Vector3 position, Vector4 color, float intensity, float radius)
{
    lightRenderer.CreatePointLight(position, color, intensity, radius);
}

void RenderManager::ClearLights()
{
    lightRenderer.ClearLights();
}
void RenderManager::UpdateGeometry(float deltaTime, Camera3D camera)
{
    geometryRenderer.Update(deltaTime, camera);
}

void RenderManager::UpdateParticles(float deltaTime, Vector3 camPos)
{
    particleSystem.Update(deltaTime, camPos);
}

void RenderManager::ResetSceneResources()
{
    lightRenderer.ClearLights();

    geometryRenderer.Shutdown();
    geometryRenderer.Init();

    grassRenderer.Shutdown();
    waterRenderer.Cleanup();
    skyboxRenderer.Unload();

    particleSystem.Shutdown();
    particleSystem.Init();
}

void RenderManager::SetupFromLevelData(const LevelData &level)
{
    TraceLog(LOG_INFO, "RenderManager: Setting up from level data '%s'...", level.name.c_str());

    ResetSceneResources();

    Vector3 sceneSunDirection = {0.3f, 0.5f, 0.8f};
    SetSunDirection(sceneSunDirection);

    lightRenderer.CreateDirectionalLight(sceneSunDirection, {1.0f, 0.95f, 0.8f, 1.0f}, 2.0f);
    lightRenderer.CreatePointLight({-5.0f, 4.0f, -5.0f}, {1.0f, 0.9f, 0.8f, 1.0f}, 12.0f, 15.0f);
    lightRenderer.CreatePointLight({5.0f, 4.0f, 5.0f}, {0.8f, 0.9f, 1.0f, 1.0f}, 12.0f, 15.0f);
    lightRenderer.CreatePointLight({0.0f, 6.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 15.0f, 20.0f);

    for (const auto &lightData : level.lights)
    {
        if (lightData.type == 0)
        {
            Vector4 colorVec = ColorNormalize(lightData.color);
            lightRenderer.CreateDirectionalLight(lightData.direction, colorVec, lightData.intensity);
        }
        else
        {
            Vector4 colorVec = ColorNormalize(lightData.color);
            lightRenderer.CreatePointLight(lightData.position, colorVec, lightData.intensity, lightData.radius);
        }
    }

    if (!level.skyboxTexture.empty())
    {
        skyboxRenderer.Load("assets/shader/skybox.vs", "assets/shader/skybox.fs");
    }

    if (level.grassBladeCount > 0 && level.grassWidth > 0.0f && level.grassLength > 0.0f)
    {
        const float areaSize = (level.grassWidth > level.grassLength) ? level.grassWidth : level.grassLength;
        InitializeGrass(level.grassBladeCount, areaSize);
        ConfigureGrass({1.0f, 0.5f}, 0.5f, 2.0f);
    }

    if (level.waterWidth > 0.0f && level.waterLength > 0.0f)
    {
        InitializeWater(level.waterWidth, level.waterLength, level.waterPosition.y);
    }

    for (const auto &pData : level.particles)
    {
        EmitterConfig config;
        config.position = pData.position;
        config.offset = pData.offset;
        config.velocity = pData.velocity;
        config.velocityRandom = pData.velocityRandom;
        config.acceleration = pData.acceleration;
        config.colorStart = pData.colorStart;
        config.colorEnd = pData.colorEnd;
        config.sizeStart = pData.sizeStart;
        config.sizeEnd = pData.sizeEnd;
        config.sizeRandom = pData.sizeRandom;
        config.lifeMin = pData.lifeMin;
        config.lifeMax = pData.lifeMax;
        config.emissionRate = pData.emissionRate;
        config.maxParticles = pData.maxParticles;

        config.blendMode = ParticleBlendMode::ALPHA;
        if (pData.blendMode == "add")
            config.blendMode = ParticleBlendMode::ADD;
        else if (pData.blendMode == "mul")
            config.blendMode = ParticleBlendMode::MULTIPLY;
        else if (pData.blendMode == "sub")
            config.blendMode = ParticleBlendMode::SUBTRACT;

        if (!pData.texturePath.empty())
        {
            config.texture = LoadTexture(pData.texturePath.c_str());
        }
        else if (!pData.textureName.empty())
        {
            config.texture = particleSystem.GetTexture(pData.textureName);
        }
        else
        {
            config.texture = particleSystem.GetTexture("soft_circle");
        }

        particleSystem.CreateEmitter(config);
    }

    // Setup camera from level data
    cameraController.Initialize(
        level.camera.startPosition,
        level.camera.startTarget,
        level.camera.startFOV);
    cameraController.SetFollowDistance(level.camera.followDistance);
    cameraController.SetFollowHeight(level.camera.followHeight);
    cameraController.SetSmoothness(level.camera.smoothness);

    TraceLog(LOG_INFO, "RenderManager: Setup complete (%d lights, %d particles)",
             (int)level.lights.size() + 4, (int)level.particles.size());
}

// NEW: Apply frame settings (culling, post-processing)
void RenderManager::ApplyFrameSettings(const FrameSettings &settings)
{
    // Apply culling margins
    geometryRenderer.SetCullingRadiusMultiplier(settings.geometryCullMargin);
    grassRenderer.SetCullingRadiusMultiplier(settings.grassCullMargin);

    // Apply post-processing settings
    EnableGrayscale(settings.grayscaleEnabled);
    EnableDepthDebug(settings.depthDebugEnabled);

    // Check if MSAA level changed
    if (settings.msaaLevel != currentFrameSettings.msaaLevel)
    {
        SetMSAALevel(settings.msaaLevel);
    }

    // Cache settings for drawing
    currentFrameSettings = settings;
}

// NEW: Unified update for all rendering systems
void RenderManager::UpdateAllSystems(float deltaTime)
{
    // Update camera
    cameraController.Update(deltaTime);
    cameraController.camera.UpdateEntity(deltaTime);

    // Update shared frustum once per frame for all culling systems
    UpdateGlobalFrustum(cameraController.camera);

    // Update all rendering subsystems with camera
    lightRenderer.Update(cameraController.camera, 64);
    grassRenderer.Update(deltaTime, cameraController.camera);
    geometryRenderer.Update(deltaTime, cameraController.camera);
    waterRenderer.Update(deltaTime, cameraController.camera);
    particleSystem.Update(deltaTime, cameraController.camera.position);
    skyboxRenderer.Update(deltaTime);
}

// =======================================
// NEW SIMPLIFIED PIPELINE METHODS
// =======================================

void RenderManager::DrawFrame(std::function<void()> sceneCallback, bool showDebug)
{
    if (!initialized)
    {
        TraceLog(LOG_ERROR, "RenderManager: Not initialized!");
        return;
    }

    // Complete frame rendering with post-processing and debug overlay
    if (postProcessingEnabled)
    {
        RenderWithPostProcessing(sceneCallback);
    }
    else
    {
        RenderDirect(sceneCallback);
    }
}

void RenderManager::SetMSAALevel(MSAALevel level)
{
    // Clamp to max supported
    if (level > maxSupportedMSAA)
    {
        TraceLog(LOG_WARNING, "RenderManager: Requested MSAA level %u exceeds max supported %u, clamping",
                 static_cast<unsigned int>(level), static_cast<unsigned int>(maxSupportedMSAA));
        level = maxSupportedMSAA;
    }

    if (currentFrameSettings.msaaLevel == level)
        return; // No change needed

    currentFrameSettings.msaaLevel = level;
    TraceLog(LOG_INFO, "RenderManager: MSAA level set to %uX", static_cast<unsigned int>(level));

    // Recreate render textures with new MSAA settings
    RecreateMSAARenderTextures();
}

void RenderManager::RecreateMSAARenderTextures()
{
    if (!initialized || !postProcessingEnabled)
        return;

    MSAALevel msaaLevel = currentFrameSettings.msaaLevel;
    unsigned int sampleCount = GetMSAASampleCount(msaaLevel);

    TraceLog(LOG_INFO, "RenderManager: Setting MSAA to %uX", sampleCount);

    if (sampleCount == 0)
    {
        // Disable MSAA
        postProcessingRenderer.DisableMSAA();
    }
    else
    {
        // Enable MSAA with specified sample count
        postProcessingRenderer.EnableMSAA(sampleCount);
    }
}
