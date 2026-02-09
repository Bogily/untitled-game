/**
 * @file LightRenderer.h
 * @brief PBR lighting system with dynamic light management
 */

#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

/**
 * @def LIGHT_DEFAULT_MAX_LIGHTS
 * @brief Default maximum lights if GPU query fails
 */
#define LIGHT_DEFAULT_MAX_LIGHTS 64

/**
 * @def LIGHT_ABSOLUTE_MAX_LIGHTS
 * @brief Absolute maximum light count
 */
#define LIGHT_ABSOLUTE_MAX_LIGHTS 1024

/**
 * @brief Light source data structure
 *
 * Aligned for GPU upload compatibility. Supports both
 * point lights and directional lights.
 */
struct alignas(16) Light
{
    int type = 1;                 ///< Light type: 1=point, 2=directional
    int enabled = 1;              ///< Enable state: 0=disabled, 1=enabled
    float pad0[2] = {0.0f, 0.0f}; ///< Padding for UBO compatibility

    Vector4 positionRadius = {0.0f, 0.0f, 0.0f, 10.0f}; ///< xyz=position, w=radius
    Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};           ///< Light color (RGBA)
    float intensity = 1.0f;                             ///< Light intensity multiplier
    float pad1[3] = {0.0f, 0.0f, 0.0f};                 ///< Padding (total: 64 bytes)
};

/**
 * @brief PBR lighting renderer with dynamic light management
 *
 * Manages physically-based rendering (PBR) shader system with support for:
 * - Dynamic point and directional lights
 * - Per-frame frustum culling and light sorting
 * - GPU capability detection for optimal light count
 * - Material properties (albedo, metallic, roughness)
 */
class LightRenderer
{
public:
    /**
     * @brief Construct a new light renderer
     */
    LightRenderer();

    /**
     * @brief Destroy light renderer and cleanup
     */
    ~LightRenderer();

    /**
     * @brief Initialize PBR shader and light system
     * @param screenWidth Screen width (currently unused)
     * @param screenHeight Screen height (currently unused)
     */
    void Init(int screenWidth, int screenHeight);

    /**
     * @brief Cleanup shader and resources
     */
    void Shutdown();

    /**
     * @brief Check if renderer is initialized
     * @return True if initialized
     */
    bool IsInitialized() const { return initialized; }

    /**
     * @brief Apply PBR shader to model with material properties
     * @param model Model to configure
     * @param albedo Base color (RGBA)
     * @param metallic Metallic factor [0,1]
     * @param roughness Roughness factor [0,1]
     */
    void ApplyToModel(Model &model, const Vector4 &albedo, float metallic, float roughness);

    /**
     * @brief Update per-frame uniforms (camera, lights)
     * @param camera Camera for position and culling
     * @param maxActiveLights Maximum active lights to upload (default 64)
     */
    void Update(const Camera &camera, int maxActiveLights = LIGHT_DEFAULT_MAX_LIGHTS);

    /**
     * @brief Create point light source
     * @param pos Light position
     * @param color Light color (RGBA)
     * @param intensity Light intensity
     * @param radius Light influence radius (default 10.0f)
     */
    void CreatePointLight(const Vector3 &pos, const Vector4 &color, float intensity, float radius = 10.0f);

    /**
     * @brief Create directional light (sun/moon)
     * @param direction Light direction vector
     * @param color Light color (RGBA)
     * @param intensity Light intensity
     */
    void CreateDirectionalLight(const Vector3 &direction, const Vector4 &color, float intensity);

    /**
     * @brief Update existing light properties
     * @param index Light index to update
     * @param pos New light position
     * @param color New light color
     * @param intensity New light intensity
     */
    void UpdateLight(int index, const Vector3 &pos, const Vector4 &color, float intensity);

    /**
     * @brief Remove all lights
     */
    void ClearLights();

    /**
     * @brief Set ambient light color
     * @param ambient Ambient light RGB
     */
    void SetAmbientLight(const Vector3 &ambient);

    /**
     * @brief Get current light count
     * @return Number of lights
     */
    int GetLightCount() const { return lightCount; }

    /**
     * @brief Get maximum supported lights
     * @return Max light count
     */
    int GetMaxLights() const { return maxLights; }

    /**
     * @brief Get sun/directional light direction
     * @return Direction vector
     */
    Vector3 GetSunDirection() const;

    /**
     * @brief Render debug visualization of lights
     */
    void DrawDebugLights();

private:
    Shader pbrShader; ///< PBR shader program

    bool initialized; ///< Initialization status

    std::vector<Light> lights; ///< All scene lights
    int lightCount;            ///< Current light count
    int maxLights;             ///< GPU-determined max lights
    Vector3 ambientLight;      ///< Ambient light color

    /**
     * @brief Upload light data to shader uniforms
     */
    void UploadLightData();

    /**
     * @brief Cull and sort lights by distance to camera
     * @param cameraPos Camera position
     * @param maxActiveLights Maximum lights to keep
     */
    void CullAndSortLights(const Vector3 &cameraPos, int maxActiveLights);

    /**
     * @brief Query GPU for maximum uniform vector count
     * @return Maximum supported light count
     */
    int QueryMaxLightsFromGPU();
};
