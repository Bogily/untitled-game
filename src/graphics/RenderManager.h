#pragma once

#include "raylib.h"
#include "raymath.h"
#include "LightRenderer.h"
#include "PostProcessingRenderer.h"
#include "SkyboxRenderer.h"
#include "GrassRenderer.h"
#include "WaterRenderer.h"
#include "GeometryRenderer.h"
#include "ParticleSystem.h"
#include "DebugRenderer.h"
#include "CameraController.h"
#include "../world/Level.h"
#include <functional>
#include <vector>

// Centralized rendering manager that handles ALL rendering operations
// This is the single source of truth for rendering state and operations
class RenderManager
{
public:
    RenderManager();
    ~RenderManager();

    // Initialization and shutdown
    void Init(int width, int height);
    void Shutdown();

    // Window management - all window operations go through here
    void SetWindowSize(int width, int height);
    void ApplyDisplayMode(int mode); // 0=Windowed, 1=Fullscreen, 2=Borderless
    int GetScreenWidth() const { return screenWidth; }
    int GetScreenHeight() const { return screenHeight; }

    // Main rendering entry point - legacy support
    void BeginFrame();
    void RenderScene(std::function<void()> sceneRenderer, Camera3D camera);
    void EndFrame();

    // New simplified rendering pipeline
    void DrawFrame(std::function<void()> sceneCallback, bool showDebug = false);

    // Rendering configuration - single place to configure all rendering settings
    void EnablePostProcessing(bool enable);
    void EnableGrayscale(bool enable);
    void EnableDepthDebug(bool enable);
    void SetSunDirection(Vector3 direction);

    bool IsPostProcessingEnabled() const { return postProcessingEnabled; }
    bool IsGrayscaleEnabled() const { return grayscaleEnabled; }
    bool IsDepthDebugEnabled() const { return depthDebugEnabled; }
    Vector3 GetSunDirection() const { return sunDirection; }

    // Subsystem access for configuration and updates
    LightRenderer *GetLightRenderer() { return &lightRenderer; }
    PostProcessingRenderer *GetPostProcessingRenderer() { return &postProcessingRenderer; }
    SkyboxRenderer *GetSkyboxRenderer() { return &skyboxRenderer; }
    GrassRenderer *GetGrassRenderer() { return &grassRenderer; }
    WaterRenderer *GetWaterRenderer() { return &waterRenderer; }
    GeometryRenderer *GetGeometryRenderer() { return &geometryRenderer; }
    ParticleSystem *GetParticleSystem() { return &particleSystem; }
    CameraController *GetCameraController() { return &cameraController; }

    // Consolidated setup from level data
    void SetupFromLevelData(const LevelData &level);

    // Apply cumulative frame settings (culling, post-processing)
    struct FrameSettings
    {
        float geometryCullMargin = 1.0f;
        float grassCullMargin = 1.70f;
        bool grayscaleEnabled = false;
        bool depthDebugEnabled = false;
        bool showDebugGrid = false;
        bool showGrass = true;
    };
    void ApplyFrameSettings(const FrameSettings &settings);

    // Unified update call for all rendering systems
    void UpdateAllSystems(float deltaTime);

    // Update methods - centralized update for all rendering subsystems
    void UpdateCamera(Camera3D camera, int maxActiveLights = 64);
    void UpdateGrass(float deltaTime, Camera3D camera);
    void UpdateWater(float deltaTime, Camera3D camera);
    void UpdateGeometry(float deltaTime, Camera3D camera);
    void UpdateParticles(float deltaTime, Vector3 camPos);
    void UpdateSkybox(float deltaTime);

    // Model shader management - apply appropriate shaders based on rendering mode
    void ApplyShaderToModel(Model &model, Vector4 albedo, float metallic, float roughness);

    // Batch apply shaders to multiple models at once
    struct ModelMaterial
    {
        Model *model;
        Vector4 albedo;
        float metallic;
        float roughness;
    };
    void ApplyShaders(const std::vector<ModelMaterial> &models);

    // Subsystem initialization helpers
    void InitializeSkybox(const char *vertexShader, const char *fragmentShader);
    void ConfigureSkybox(Vector3 skyColor, Vector3 cloudColor, Vector3 sunColor);
    void InitializeGrass(int bladeCount, float area);
    void ConfigureGrass(Vector2 windDirection, float windStrength, float windSpeed);
    void InitializeWater(float width, float height, float waterLevel);

    // Light management - centralized light configuration
    void CreateDirectionalLight(Vector3 direction, Vector4 color, float intensity);
    void CreatePointLight(Vector3 position, Vector4 color, float intensity, float radius = 10.0f);
    void ClearLights();

    bool IsInitialized() const { return initialized; }

private:
    // Screen configuration
    int screenWidth;
    int screenHeight;
    int currentDisplayMode; // 0=Windowed, 1=Fullscreen, 2=Borderless

    // Rendering state
    bool initialized;
    bool postProcessingEnabled;
    bool grayscaleEnabled;
    bool depthDebugEnabled;
    Vector3 sunDirection;

    // Rendering subsystems - all managed here (standardized naming)
    LightRenderer lightRenderer;
    PostProcessingRenderer postProcessingRenderer;
    SkyboxRenderer skyboxRenderer;
    GrassRenderer grassRenderer;
    GeometryRenderer geometryRenderer;
    WaterRenderer waterRenderer;
    ParticleSystem particleSystem;
    DebugRenderer debugRenderer;
    CameraController cameraController;

    // Frame settings cache
    FrameSettings currentFrameSettings;

    // Internal rendering methods
    void RenderDirect(std::function<void()> sceneRenderer);
    void RenderWithPostProcessing(std::function<void()> sceneRenderer);
};
