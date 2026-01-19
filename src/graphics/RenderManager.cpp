#include "RenderManager.h"

RenderManager::RenderManager()
    : screenWidth(0),
      screenHeight(0),
      currentDisplayMode(0),
      initialized(false),
      postProcessingEnabled(true),
      grayscaleEnabled(false),
      sunDirection({0.3f, 0.5f, 0.8f})
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
    lightRenderer.Init(width, height);
    postProcessingRenderer.Init(width, height);
    geometryRenderer.Init();
    particleSystem.Init();

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

    postProcessingRenderer.Shutdown();
    postProcessingRenderer.Init(width, height);

    // Reapply settings
    postProcessingRenderer.SetGrayscaleEnabled(grayscaleEnabled);

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
