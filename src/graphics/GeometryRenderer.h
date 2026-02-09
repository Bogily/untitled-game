/**
 * @file GeometryRenderer.h
 * @brief Batched geometry renderer with GPU culling
 */

#pragma once
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "Frustum.h"
#include <vector>
#include <unordered_map>
#include <string>

/**
 * @brief Instanced geometry renderer with frustum culling
 *
 * Manages multiple model types and instances with:
 * - GPU compute shader frustum culling (optional)
 * - CPU fallback culling
 * - Per-instance transforms (position, rotation, scale)
 * - Automatic AABB calculation and culling
 */
class GeometryRenderer
{
public:
    /**
     * @brief Construct geometry renderer
     */
    GeometryRenderer();

    /**
     * @brief Destroy geometry renderer
     */
    ~GeometryRenderer();

    /**
     * @brief Initialize renderer and GPU resources
     */
    void Init();

    /**
     * @brief Update renderer state
     * @param deltaTime Time elapsed since last frame
     * @param camera Camera for culling
     */
    void Update(float deltaTime, Camera3D camera);

    /**
     * @brief Shutdown and cleanup resources
     */
    void Shutdown();

    /**
     * @brief Register model type for instancing
     * @param name Model identifier
     * @param model Pointer to model (must remain valid)
     * @return Model ID for AddInstance calls
     */
    int RegisterModel(const std::string &name, Model *model);

    /**
     * @brief Add instance of registered model
     * @param modelID Model ID from RegisterModel
     * @param position World position
     * @param scale Uniform scale factor (default 1.0)
     * @param rotation Euler rotation angles in degrees (default {0,0,0})
     */
    void AddInstance(int modelID, Vector3 position, float scale = 1.0f, Vector3 rotation = {0, 0, 0});

    /**
     * @brief Clear all instances
     *
     * Call at start of frame if instances are dynamic.
     */
    void ClearInstances();

    /**
     * @brief Render all visible instances
     * @param camera Camera for rendering
     */
    void Draw(Camera3D camera);

    /**
     * @brief Get visible instance count after culling
     * @return Number of visible instances
     */
    int GetVisibleCount() const { return visibleCount; }

    /**
     * @brief Get total instance count
     * @return Total instances before culling
     */
    int GetTotalCount() const { return totalInstanceCount; }

    /**
     * @brief Enable or disable GPU compute culling
     * @param enabled True to use GPU culling, false for CPU
     */
    void SetGPUCullingEnabled(bool enabled) { gpuCullingEnabled = enabled; }

    /**
     * @brief Check if GPU culling is enabled
     * @return True if GPU culling is active
     */
    bool IsGPUCullingEnabled() const { return gpuCullingEnabled; }

    /**
     * @brief Set culling radius multiplier
     * @param m Multiplier (>1.0 keeps objects visible longer)
     */
    void SetCullingRadiusMultiplier(float m) { cullRadiusMultiplier = m; }

    /**
     * @brief Get culling radius multiplier
     * @return Current multiplier value
     */
    float GetCullingRadiusMultiplier() const { return cullRadiusMultiplier; }

private:
    /**
     * @brief Instance data for GPU/CPU culling (16-byte aligned)
     */
    struct ModelInstance
    {
        float posX, posY, posZ, scale;                  ///< Position and scale
        float rotX, rotY, rotZ, pad0;                   ///< Euler angles (degrees)
        float boundsMinX, boundsMinY, boundsMinZ, pad1; ///< AABB min
        float boundsMaxX, boundsMaxY, boundsMaxZ, pad2; ///< AABB max
        unsigned int modelID;                           ///< Model type ID
        unsigned int pad3, pad4, pad5;                  ///< Padding to 16-byte
    };

    /**
     * @brief Registered model type
     */
    struct RegisteredModel
    {
        Model *model;       ///< Pointer to model
        std::string name;   ///< Model identifier
        BoundingBox bounds; ///< Local-space AABB
    };

    std::vector<RegisteredModel> registeredModels;      ///< All registered models
    std::unordered_map<std::string, int> modelNameToID; ///< Name to ID mapping

    std::vector<ModelInstance> allInstances;  ///< All instances
    std::vector<unsigned int> visibleIndices; ///< Visible indices (CPU)

    unsigned int computeProgram;     ///< Compute shader program
    unsigned int ssboAllInstances;   ///< SSBO for all instances
    unsigned int ssboVisibleIndices; ///< SSBO for visible indices
    bool gpuCullingEnabled;          ///< GPU culling toggle
    float cullRadiusMultiplier;      ///< Culling radius factor (default 1.1)

    int visibleCount;       ///< Visible instance count
    int totalInstanceCount; ///< Total instance count

    /**
     * @brief Extract view frustum from camera
     * @param camera Camera to extract from
     * @return Frustum planes
     */
    Frustum ExtractFrustum(Camera3D camera);

    /**
     * @brief Test if AABB intersects frustum
     * @param frustum View frustum
     * @param worldAABB World-space bounding box
     * @return True if visible
     */
    bool IsAABBInFrustum(const Frustum &frustum, BoundingBox worldAABB);

    /**
     * @brief Run GPU compute shader culling
     * @param camera Camera for frustum extraction
     */
    void RunGPUCulling(Camera3D camera);

    /**
     * @brief Run CPU fallback culling
     * @param camera Camera for frustum extraction
     */
    void RunCPUCulling(Camera3D camera);

    /**
     * @brief Compile compute shader program
     * @param path Shader file path
     * @return Compute program ID
     */
    unsigned int CompileComputeProgram(const char *path);
};
