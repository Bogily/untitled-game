/**
 * @file RenderManager.h
 * @brief Centralized rendering system coordinator
 */

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

/**
 * @brief Centralized rendering manager coordinating all render systems
 *
 * RenderManager is the single source of truth for all rendering operations.
 * It manages:
 * - Lighting system (PBR shaders, directional/point lights)
 * - Post-processing effects (grayscale, depth debug)
 * - Environment rendering (skybox, grass, water)
 * - Geometry and particle systems
 * - Window and display mode management
 *
 * All rendering configuration and execution flows through this manager.
 */
class RenderManager
{
public:
    /**
     * @brief Construct a new render manager
     */
    RenderManager();

    /**
     * @brief Destroy render manager and cleanup resources
     */
    ~RenderManager();

    /**
     * @brief Initialize all rendering subsystems
     * @param width Screen width
     * @param height Screen height
     */
    void Init(int width, int height);

    /**
     * @brief Shutdown and cleanup all rendering subsystems
     */
    void Shutdown();

    /**
     * @brief Update window size and reinitialize subsystems
     * @param width New screen width
     * @param height New screen height
     */
    void SetWindowSize(int width, int height);

    /**
     * @brief Apply display mode (windowed, fullscreen, borderless)
     * @param mode Display mode: 0=Windowed, 1=Fullscreen, 2=Borderless
     */
    void ApplyDisplayMode(int mode);

    /**
     * @brief Get current screen width
     * @return Screen width in pixels
     */
    int GetScreenWidth() const { return screenWidth; }

    /**
     * @brief Get current screen height
     * @return Screen height in pixels
     */
    int GetScreenHeight() const { return screenHeight; }

    /**
     * @brief Begin frame rendering (legacy support)
     */
    void BeginFrame();

    /**
     * @brief Render scene with camera (legacy support)
     * @param sceneRenderer Callback to render scene content
     * @param camera Camera for rendering
     */
    void RenderScene(std::function<void()> sceneRenderer, Camera3D camera);

    /**
     * @brief End frame rendering (legacy support)
     */
    void EndFrame();

    /**
     * @brief Simplified rendering pipeline - single call
     * @param sceneCallback Callback to render scene content
     * @param showDebug Whether to render debug visualization
     */
    void DrawFrame(std::function<void()> sceneCallback, bool showDebug = false);

    /**
     * @brief Enable or disable post-processing effects
     * @param enable True to enable post-processing
     */
    void EnablePostProcessing(bool enable);

    /**
     * @brief Enable or disable grayscale effect
     * @param enable True to enable grayscale
     */
    void EnableGrayscale(bool enable);

    /**
     * @brief Enable or disable depth buffer visualization
     * @param enable True to show depth buffer
     */
    void EnableDepthDebug(bool enable);

    /**
     * @brief Set directional light (sun) direction
     * @param direction Normalized light direction vector
     */
    void SetSunDirection(Vector3 direction);

    /**
     * @brief Check if post-processing is enabled
     * @return True if enabled
     */
    bool IsPostProcessingEnabled() const { return postProcessingEnabled; }

    /**
     * @brief Check if grayscale effect is enabled
     * @return True if enabled
     */
    bool IsGrayscaleEnabled() const { return grayscaleEnabled; }

    /**
     * @brief Check if depth debug is enabled
     * @return True if enabled
     */
    bool IsDepthDebugEnabled() const { return depthDebugEnabled; }

    /**
     * @brief Get current sun direction
     * @return Sun direction vector
     */
    Vector3 GetSunDirection() const { return sunDirection; }

    /**
     * @brief Get light renderer subsystem
     * @return Pointer to light renderer
     */
    LightRenderer *GetLightRenderer() { return &lightRenderer; }

    /**
     * @brief Get post-processing renderer subsystem
     * @return Pointer to post-processing renderer
     */
    PostProcessingRenderer *GetPostProcessingRenderer() { return &postProcessingRenderer; }

    /**
     * @brief Get skybox renderer subsystem
     * @return Pointer to skybox renderer
     */
    SkyboxRenderer *GetSkyboxRenderer() { return &skyboxRenderer; }

    /**
     * @brief Get grass renderer subsystem
     * @return Pointer to grass renderer
     */
    GrassRenderer *GetGrassRenderer() { return &grassRenderer; }

    /**
     * @brief Get water renderer subsystem
     * @return Pointer to water renderer
     */
    WaterRenderer *GetWaterRenderer() { return &waterRenderer; }

    /**
     * @brief Get geometry renderer subsystem
     * @return Pointer to geometry renderer
     */
    GeometryRenderer *GetGeometryRenderer() { return &geometryRenderer; }

    /**
     * @brief Get particle system
     * @return Pointer to particle system
     */
    ParticleSystem *GetParticleSystem() { return &particleSystem; }

    /**
     * @brief Get camera controller
     * @return Pointer to camera controller
     */
    CameraController *GetCameraController() { return &cameraController; }

    /**
     * @brief Setup rendering systems from level data
     * @param level Level configuration data
     */
    void SetupFromLevelData(const LevelData &level);

    /**
     * @brief Per-frame rendering configuration
     */
    struct FrameSettings
    {
        float geometryCullMargin = 1.0f; ///< Geometry frustum culling margin
        float grassCullMargin = 1.70f;   ///< Grass frustum culling margin
        bool grayscaleEnabled = false;   ///< Grayscale post-processing
        bool depthDebugEnabled = false;  ///< Depth buffer visualization
        bool showDebugGrid = false;      ///< Debug grid rendering
        bool showGrass = true;           ///< Grass rendering toggle
    };

    /**
     * @brief Apply frame settings for current frame
     * @param settings Frame configuration
     */
    void ApplyFrameSettings(const FrameSettings &settings);

    /**
     * @brief Update all rendering subsystems
     * @param deltaTime Time elapsed since last frame
     */
    void UpdateAllSystems(float deltaTime);

    /**
     * @brief Update camera state and affected systems
     * @param camera Camera to use
     * @param maxActiveLights Maximum active lights (default 64)
     */
    void UpdateCamera(Camera3D camera, int maxActiveLights = 64);

    /**
     * @brief Update grass rendering system
     * @param deltaTime Time elapsed
     * @param camera Camera for frustum culling
     */
    void UpdateGrass(float deltaTime, Camera3D camera);

    /**
     * @brief Update water simulation and rendering
     * @param deltaTime Time elapsed
     * @param camera Camera position
     */
    void UpdateWater(float deltaTime, Camera3D camera);

    /**
     * @brief Update geometry rendering system
     * @param deltaTime Time elapsed
     * @param camera Camera for frustum culling
     */
    void UpdateGeometry(float deltaTime, Camera3D camera);

    /**
     * @brief Update particle systems
     * @param deltaTime Time elapsed
     * @param camPos Camera position for sorting
     */
    void UpdateParticles(float deltaTime, Vector3 camPos);

    /**
     * @brief Update skybox rendering
     * @param deltaTime Time elapsed
     */
    void UpdateSkybox(float deltaTime);

    /**
     * @brief Apply PBR shader to model with material properties
     * @param model Model to configure
     * @param albedo Base color (RGBA)
     * @param metallic Metallic factor [0,1]
     * @param roughness Roughness factor [0,1]
     */
    void ApplyShaderToModel(Model &model, Vector4 albedo, float metallic, float roughness);

    /**
     * @brief Model material configuration
     */
    struct ModelMaterial
    {
        Model *model;    ///< Pointer to model
        Vector4 albedo;  ///< Base color
        float metallic;  ///< Metallic factor
        float roughness; ///< Roughness factor
    };

    /**
     * @brief Apply shaders to multiple models
     * @param models Vector of model material configurations
     */
    void ApplyShaders(const std::vector<ModelMaterial> &models);

    /**
     * @brief Initialize skybox rendering
     * @param vertexShader Path to vertex shader
     * @param fragmentShader Path to fragment shader
     */
    void InitializeSkybox(const char *vertexShader, const char *fragmentShader);

    /**
     * @brief Configure skybox colors
     * @param skyColor Sky base color
     * @param cloudColor Cloud color
     * @param sunColor Sun/light color
     */
    void ConfigureSkybox(Vector3 skyColor, Vector3 cloudColor, Vector3 sunColor);

    /**
     * @brief Initialize grass rendering system
     * @param bladeCount Number of grass blades
     * @param area Total grass area
     */
    void InitializeGrass(int bladeCount, float area);

    /**
     * @brief Configure grass wind simulation
     * @param windDirection Wind direction vector
     * @param windStrength Wind strength multiplier
     * @param windSpeed Wind animation speed
     */
    void ConfigureGrass(Vector2 windDirection, float windStrength, float windSpeed);

    /**
     * @brief Initialize water simulation and rendering
     * @param width Water plane width
     * @param height Water plane height
     * @param waterLevel Y-axis water level
     */
    void InitializeWater(float width, float height, float waterLevel);

    /**
     * @brief Create directional light (sun/moon)
     * @param direction Light direction vector
     * @param color Light color (RGBA)
     * @param intensity Light intensity multiplier
     */
    void CreateDirectionalLight(Vector3 direction, Vector4 color, float intensity);

    /**
     * @brief Create point light source
     * @param position Light world position
     * @param color Light color (RGBA)
     * @param intensity Light intensity multiplier
     * @param radius Light influence radius (default 10.0f)
     */
    void CreatePointLight(Vector3 position, Vector4 color, float intensity, float radius = 10.0f);

    /**
     * @brief Remove all lights from scene
     */
    void ClearLights();

    /**
     * @brief Check if rendering system is initialized
     * @return True if initialized
     */
    bool IsInitialized() const { return initialized; }

private:
    int screenWidth;        ///< Current screen width
    int screenHeight;       ///< Current screen height
    int currentDisplayMode; ///< Display mode: 0=Windowed, 1=Fullscreen, 2=Borderless

    bool initialized;           ///< Initialization status
    bool postProcessingEnabled; ///< Post-processing toggle
    bool grayscaleEnabled;      ///< Grayscale effect toggle
    bool depthDebugEnabled;     ///< Depth visualization toggle
    Vector3 sunDirection;       ///< Directional light direction

    LightRenderer lightRenderer;                   ///< PBR lighting system
    PostProcessingRenderer postProcessingRenderer; ///< Post-processing pipeline
    SkyboxRenderer skyboxRenderer;                 ///< Skybox/environment rendering
    GrassRenderer grassRenderer;                   ///< Grass rendering system
    GeometryRenderer geometryRenderer;             ///< Static geometry rendering
    WaterRenderer waterRenderer;                   ///< Water simulation and rendering
    ParticleSystem particleSystem;                 ///< Particle effects
    DebugRenderer debugRenderer;                   ///< Debug visualization
    CameraController cameraController;             ///< Camera control system

    FrameSettings currentFrameSettings; ///< Cached frame configuration

    /**
     * @brief Render scene directly without post-processing
     * @param sceneRenderer Scene rendering callback
     */
    void RenderDirect(std::function<void()> sceneRenderer);

    /**
     * @brief Render scene with post-processing pipeline
     * @param sceneRenderer Scene rendering callback
     */
    void RenderWithPostProcessing(std::function<void()> sceneRenderer);
};
