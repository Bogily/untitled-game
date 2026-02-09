/**
 * @file GrassRenderer.h
 * @brief Instanced grass renderer with wind animation
 */

#pragma once
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "Frustum.h"
#include <vector>

/**
 * @brief Grass renderer with instancing and wind animation
 *
 * Renders large quantities of grass blades using:
 * - GPU instancing for performance
 * - Vertex shader wind animation
 * - CPU frustum culling with spatial grid acceleration
 * - Optional GPU compute culling
 * - Double-buffered VBOs to reduce stalls
 */
class GrassRenderer
{
public:
    /**
     * @brief Construct grass renderer
     */
    GrassRenderer();

    /**
     * @brief Destroy grass renderer
     */
    ~GrassRenderer();

    /**
     * @brief Initialize grass with random positions
     * @param grassCount Number of grass instances
     * @param areaSize Size of grass area in world units
     */
    void Init(int grassCount, float areaSize);

    /**
     * @brief Update grass animation and culling
     * @param deltaTime Time elapsed since last frame
     * @param camera Camera for frustum culling
     */
    void Update(float deltaTime, Camera3D camera);

    /**
     * @brief Render visible grass instances
     * @param camera Camera for rendering
     */
    void Draw(Camera3D camera);

    /**
     * @brief Shutdown and cleanup resources
     */
    void Shutdown();

    /**
     * @brief Get visible grass count after culling
     * @return Number of visible instances
     */
    int GetVisibleCount() const { return visibleCount; }

    /**
     * @brief Get total grass count
     * @return Total grass instances
     */
    int GetTotalCount() const { return totalGrassCount; }

    /**
     * @brief Set wind direction
     * @param direction 2D wind direction vector
     */
    void SetWindDirection(Vector2 direction) { windDirection = direction; }

    /**
     * @brief Set wind strength
     * @param strength Wind force magnitude
     */
    void SetWindStrength(float strength) { windStrength = strength; }

    /**
     * @brief Set wind animation speed
     * @param speed Wind speed multiplier
     */
    void SetWindSpeed(float speed) { windSpeed = speed; }

    /**
     * @brief Set FOV culling multiplier
     * @param multiplier Multiplier (>1.0 = less aggressive culling)
     */
    void SetFOVCullingMultiplier(float multiplier) { fovCullingMultiplier = multiplier; }

    /**
     * @brief Set culling radius multiplier
     * @param m Multiplier for culling radius
     */
    void SetCullingRadiusMultiplier(float m) { cullRadiusMultiplier = m; }

    /**
     * @brief Get culling radius multiplier
     * @return Current multiplier value
     */
    float GetCullingRadiusMultiplier() const { return cullRadiusMultiplier; }

private:
    /**
     * @brief Grass instance data (position and scale)
     */
    struct InstanceData
    {
        float x, y, z; ///< World position
        float scale;   ///< Scale factor
    };

    Shader grassShader;     ///< Grass rendering shader
    Mesh grassBladeMesh;    ///< Single blade mesh (instanced)
    Material grassMaterial; ///< Grass material

    std::vector<InstanceData> allInstances;     ///< All grass positions
    std::vector<InstanceData> visibleInstances; ///< Visible after culling
    unsigned int instanceVBO;                   ///< Legacy single VBO
    unsigned int instanceVBOs[2];               ///< Double-buffered VBOs
    int currentVBOIndex;                        ///< Current VBO index

    int timeLoc;             ///< Time uniform location
    int windDirLoc;          ///< Wind direction uniform location
    int windStrengthLoc;     ///< Wind strength uniform location
    int windSpeedLoc;        ///< Wind speed uniform location
    int viewPosLoc;          ///< View position uniform location
    int matViewLoc;          ///< View matrix uniform location
    int matProjLoc;          ///< Projection matrix uniform location
    int lightDirLoc;         ///< Light direction uniform location
    int lightColorLoc;       ///< Light color uniform location
    int grassColorTopLoc;    ///< Grass top color uniform location
    int grassColorBottomLoc; ///< Grass bottom color uniform location
    int ambientStrengthLoc;  ///< Ambient strength uniform location

    Vector2 windDirection; ///< Wind direction vector
    float windStrength;    ///< Wind force magnitude
    float windSpeed;       ///< Wind animation speed
    float currentTime;     ///< Animation time accumulator

    float fovCullingMultiplier; ///< FOV culling factor (>1.0 = less aggressive)

    int visibleCount;    ///< Visible instance count
    int totalGrassCount; ///< Total grass count
    float areaSize;      ///< Grass area size

    /**
     * @brief Create grass blade mesh geometry
     */
    void CreateGrassBladeMesh();

    /**
     * @brief Generate random grass positions
     * @param count Number of instances
     * @param size Area size
     */
    void GenerateGrassPositions(int count, float size);

    /**
     * @brief Setup instance VBO
     */
    void SetupInstanceBuffer();

    /**
     * @brief Extract frustum from camera
     * @param camera Camera to extract from
     * @return View frustum
     */
    Frustum ExtractFrustum(Camera3D camera);

    /**
     * @brief Test if point is in frustum
     * @param frustum View frustum
     * @param point Test point
     * @param radius Culling radius
     * @return True if visible
     */
    bool IsPointInFrustum(const Frustum &frustum, Vector3 point, float radius);

    int gridCols;                            ///< Spatial grid columns
    int gridRows;                            ///< Spatial grid rows
    float gridCellSize;                      ///< Grid cell size
    float gridOriginX;                       ///< Grid origin X
    float gridOriginZ;                       ///< Grid origin Z
    std::vector<std::vector<int>> gridCells; ///< Spatial grid cells

    /**
     * @brief Build spatial acceleration grid
     */
    void BuildSpatialGrid();

    int lastUploadedCount; ///< Last uploaded instance count
    double updateTimeMs;   ///< Update time in milliseconds
    double drawTimeMs;     ///< Draw time in milliseconds

    unsigned int computeProgram;       ///< GPU culling compute program
    unsigned int ssboAllInstances;     ///< All instances SSBO
    unsigned int ssboVisibleInstances; ///< Visible instances SSBO
    bool gpuCullingEnabled;            ///< GPU culling toggle
    float cullRadiusMultiplier;        ///< Culling radius multiplier
};
