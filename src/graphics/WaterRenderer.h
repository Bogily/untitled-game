/**
 * @file WaterRenderer.h
 * @brief Water surface rendering with optional GPU simulation
 */

#pragma once

#include <raylib.h>
#include <raymath.h>

/**
 * @brief Water surface renderer with wave simulation
 *
 * Renders realistic water surface with:
 * - Wave animation and normal mapping
 * - Foam effects and specular highlights
 * - Optional compute shader-based physics simulation
 * - Configurable wave parameters and wind
 */
class WaterRenderer
{
public:
    /**
     * @brief Construct water renderer
     */
    WaterRenderer();

    /**
     * @brief Destroy water renderer and cleanup
     */
    ~WaterRenderer();

    /**
     * @brief Initialize water mesh and shaders
     */
    void Init();

    /**
     * @brief Update water simulation and animation
     * @param deltaTime Time elapsed since last frame
     * @param camera Camera for view-dependent effects
     */
    void Update(float deltaTime, Camera3D camera);

    /**
     * @brief Render water surface
     */
    void Draw();

    /**
     * @brief Cleanup water resources
     */
    void Cleanup();

    /**
     * @brief Set light direction for specular highlights
     * @param direction Normalized light direction
     */
    void SetLightDirection(Vector3 direction);

    /**
     * @brief Set water surface Y position
     * @param yPosition Height in world units
     */
    void SetWaterLevel(float yPosition);

    /**
     * @brief Set water plane dimensions
     * @param width Width in world units
     * @param depth Depth in world units
     */
    void SetWaterSize(float width, float depth);

    /**
     * @brief Enable or disable compute shader simulation
     * @param enable True to use GPU compute for water physics
     */
    void SetUseComputeShader(bool enable);

private:
    /**
     * @brief Initialize compute shader for GPU simulation
     */
    void InitComputeShader();

    /**
     * @brief Run compute shader water physics
     * @param deltaTime Time step for simulation
     */
    void RunComputeShader(float deltaTime);

    /**
     * @brief Update mesh vertices from compute shader results
     */
    void UpdateMeshFromCompute();

    Shader waterShader;     ///< Water rendering shader
    Mesh waterMesh;         ///< Water surface mesh
    Model waterModel;       ///< Water model
    Material waterMaterial; ///< Water material

    float waterLevel;  ///< Y position of water surface
    float waterWidth;  ///< Water plane width
    float waterDepth;  ///< Water plane depth
    float elapsedTime; ///< Animation time accumulator

    Vector3 lightDirection; ///< Light direction for specular

    int timeLoc;          ///< Time uniform location
    int viewPosLoc;       ///< View position uniform location
    int lightDirLoc;      ///< Light direction uniform location
    int mvpLoc;           ///< MVP matrix uniform location
    int matModelLoc;      ///< Model matrix uniform location
    int matNormalLoc;     ///< Normal matrix uniform location
    int normalScaleLoc;   ///< Normal scale uniform location
    int foamThresholdLoc; ///< Foam threshold uniform location
    int foamIntensityLoc; ///< Foam intensity uniform location
    int glossinessLoc;    ///< Glossiness uniform location

    unsigned int computeProgram;      ///< Compute shader program
    unsigned int ssboHeightField;     ///< Height field SSBO
    unsigned int ssboVelocityField;   ///< Velocity field SSBO
    unsigned int ssboPrevHeightField; ///< Previous height field SSBO

    int gridWidth;         ///< Simulation grid width
    int gridHeight;        ///< Simulation grid height
    bool useComputeShader; ///< Whether to use GPU simulation

    int compDeltaTimeLoc;     ///< Compute deltaTime uniform location
    int compTimeLoc;          ///< Compute time uniform location
    int compGridWidthLoc;     ///< Compute grid width uniform location
    int compGridHeightLoc;    ///< Compute grid height uniform location
    int compDampingLoc;       ///< Compute damping uniform location
    int compWaveSpeedLoc;     ///< Compute wave speed uniform location
    int compWaveStrengthLoc;  ///< Compute wave strength uniform location
    int compWindDirectionLoc; ///< Compute wind direction uniform location
    int compWindStrengthLoc;  ///< Compute wind strength uniform location

    float damping;         ///< Wave damping factor
    float waveSpeed;       ///< Wave propagation speed
    float waveStrength;    ///< Wave amplitude multiplier
    Vector2 windDirection; ///< Wind direction vector
    float windStrength;    ///< Wind force magnitude

    float normalScale;   ///< Normal map intensity
    float foamThreshold; ///< Foam appearance threshold
    float foamIntensity; ///< Foam brightness
    float glossiness;    ///< Surface glossiness/shininess

    bool initialized; ///< Initialization status
};
