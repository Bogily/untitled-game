/**
 * @file SkyboxRenderer.h
 * @brief Procedural skybox rendering with volumetric clouds
 */

#pragma once
#include "raylib.h"

/**
 * @brief Skybox renderer with procedural sky and 3D volumetric clouds
 *
 * Renders animated procedural skybox with customizable sky colors,
 * sun direction, and volumetric cloud simulation.
 */
class SkyboxRenderer
{
private:
    Shader shader;       ///< Skybox shader
    Model cube;          ///< Cube model for skybox
    int timeLoc;         ///< Time uniform location
    int skyColorLoc;     ///< Sky color uniform location
    int cloudColorLoc;   ///< Cloud color uniform location
    int sunDirectionLoc; ///< Sun direction uniform location
    int sunColorLoc;     ///< Sun color uniform location

    int cloudDensityLoc;  ///< Cloud density uniform location
    int cloudHeightLoc;   ///< Cloud height uniform location
    int cloudScaleLoc;    ///< Cloud scale uniform location
    int cloudSpeedLoc;    ///< Cloud speed uniform location
    int cloudCoverageLoc; ///< Cloud coverage uniform location
    int cloudOffsetLoc;   ///< Cloud offset uniform location

    float time;           ///< Elapsed time for animation
    Vector3 skyColor;     ///< Sky base color
    Vector3 cloudColor;   ///< Cloud color
    Vector3 sunDirection; ///< Directional light direction
    Vector3 sunColor;     ///< Sun/light color

    float cloudDensity;  ///< Cloud thickness/opacity [0,1]
    float cloudHeight;   ///< Cloud layer altitude (world units)
    float cloudScale;    ///< Cloud detail scale (larger = bigger)
    float cloudSpeed;    ///< Animation speed multiplier
    float cloudCoverage; ///< Cloud coverage [0,1]
    Vector3 cloudOffset; ///< 3D offset for cloud animation
    bool loaded;         ///< Resource load state

public:
    /**
     * @brief Construct skybox renderer
     */
    SkyboxRenderer();

    /**
     * @brief Destroy skybox renderer
     */
    ~SkyboxRenderer();

    /**
     * @brief Load skybox shaders
     * @param vsPath Vertex shader path
     * @param fsPath Fragment shader path
     */
    void Load(const char *vsPath, const char *fsPath);

    /**
     * @brief Set sky base color
     * @param color RGB sky color
     */
    void SetSkyColor(Vector3 color);

    /**
     * @brief Set cloud color
     * @param color RGB cloud color
     */
    void SetCloudColor(Vector3 color);

    /**
     * @brief Set sun/light direction
     * @param direction Normalized direction vector
     */
    void SetSunDirection(Vector3 direction);

    /**
     * @brief Set sun/light color
     * @param color RGB sun color
     */
    void SetSunColor(Vector3 color);

    /**
     * @brief Set cloud density/opacity
     * @param density Cloud thickness [0,1]
     */
    void SetCloudDensity(float density);

    /**
     * @brief Set cloud layer altitude
     * @param height Altitude in world units
     */
    void SetCloudHeight(float height);

    /**
     * @brief Set cloud detail scale
     * @param scale Larger values = bigger clouds
     */
    void SetCloudScale(float scale);

    /**
     * @brief Set cloud animation speed
     * @param speed Speed multiplier
     */
    void SetCloudSpeed(float speed);

    /**
     * @brief Set cloud coverage amount
     * @param coverage Coverage factor [0,1]
     */
    void SetCloudCoverage(float coverage);

    /**
     * @brief Update cloud animation
     * @param deltaTime Time elapsed since last frame
     */
    void Update(float deltaTime);

    /**
     * @brief Render skybox
     * @param camera Camera for positioning
     */
    void Draw(Camera3D camera);

    /**
     * @brief Unload skybox resources
     */
    void Unload();
};
