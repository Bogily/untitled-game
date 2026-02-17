/**
 * @file SDFCollisionSystem.h
 * @brief GPU-accelerated collision system using Signed Distance Fields
 *
 * Automatically generates a 3D signed distance field from scene geometry
 * (meshes, primitives) using a compute shader, then queries it each frame
 * to resolve entity-vs-world collisions without manually placed colliders.
 */

#pragma once

#include "raylib.h"
#include "raymath.h"
#include "../world/Level.h"
#include "../graphics/Frustum.h"
#include <vector>
#include <unordered_map>
#include <string>

/**
 * @brief Result of a single collision query against the SDF
 */
struct SDFCollisionResult
{
    Vector3 pushVector;     ///< Direction and magnitude to push entity out of geometry
    Vector3 surfaceNormal;  ///< Surface normal at the closest contact point
    float penetrationDepth; ///< How far the entity sphere penetrates geometry (0 if none)
    bool colliding;         ///< True if the entity sphere overlaps scene geometry
};

/**
 * @brief GPU-accelerated signed distance field collision system
 *
 * Pipeline overview:
 * 1. **BuildSDF()** (scene load) – extracts world-space triangles from every
 *    static scene object, uploads them to an SSBO, then dispatches a 3D compute
 *    shader that writes a signed distance into each voxel of a 3D R16F texture.
 * 2. **QueryCollision() / QueryCollisionBatch()** (per frame) – dispatches a
 *    second compute shader that reads the SDF texture with trilinear filtering,
 *    computes the gradient via central differences, and writes back per-entity
 *    push vectors through an SSBO.
 * 3. **ResolvePosition()** – convenience wrapper that iteratively queries and
 *    pushes an entity sphere out of overlapping geometry.
 *
 * The SDF is signed: positive values are outside geometry, negative values are
 * inside.  The gradient of the SDF always points outward, which gives the
 * surface normal used for push-back.
 */
class SDFCollisionSystem
{
public:
    SDFCollisionSystem();
    ~SDFCollisionSystem();

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Compile compute shaders and allocate GPU resources
     *
     * Must be called after GLAD / OpenGL context is ready.
     */
    void Init();

    /**
     * @brief Release all GPU resources
     */
    void Shutdown();

    // -----------------------------------------------------------------------
    // SDF construction
    // -----------------------------------------------------------------------

    /**
     * @brief Build the signed distance field from scene geometry
     *
     * Extracts triangles from every **static** object in the scene, transforms
     * them into world space, uploads them to the GPU, and runs the SDF
     * generation compute shader.  Also keeps a CPU-side copy of the SDF for
     * fast single-entity queries without a GPU round-trip.
     *
     * @param objects     Scene object descriptions (position, rotation, scale, modelType)
     * @param sceneModels Map of modelType -> loaded Model (must stay valid)
     */
    void BuildSDF(const std::vector<LevelData::ObjectData> &objects,
                  const std::unordered_map<std::string, Model> &sceneModels);

    // -----------------------------------------------------------------------
    // Collision queries
    // -----------------------------------------------------------------------

    /**
     * @brief Query collision for a single sphere (CPU-side, uses cached SDF)
     *
     * @param position Center of the collision sphere in world space
     * @param radius   Radius of the collision sphere
     * @return Collision result with push vector and surface normal
     */
    SDFCollisionResult QueryCollision(Vector3 position, float radius) const;

    /**
     * @brief Batch-query collisions on the GPU for multiple entities
     *
     * Dispatches the collision compute shader and reads results back via SSBO.
     *
     * @param positions Array of entity center positions
     * @param radii     Array of entity collision radii
     * @param results   Output array (must be pre-allocated to \p count elements)
     * @param count     Number of entities
     */
    void QueryCollisionBatch(const Vector3 *positions, const float *radii,
                             SDFCollisionResult *results, int count);

    /**
     * @brief Iteratively resolve a sphere out of overlapping geometry
     *
     * Calls QueryCollision() up to \p maxIterations times, each time pushing
     * the position along the SDF gradient.
     *
     * @param position      Starting position (modified in place)
     * @param radius        Collision sphere radius
     * @param maxIterations Maximum push iterations (default 4)
     * @return Final resolved position
     */
    Vector3 ResolvePosition(Vector3 position, float radius, int maxIterations = 4) const;

    // -----------------------------------------------------------------------
    // Debug visualisation
    // -----------------------------------------------------------------------

    /**
     * @brief Draw a horizontal cross-section of the SDF as colored quads
     *
     * Blue = positive (outside), Red = negative (inside), brighter = closer.
     *
     * @param camera  Current camera (for 3D drawing context)
     * @param yLevel  World-space Y coordinate of the slice
     */
    void DrawDebugSlice(Camera3D camera, float yLevel) const;

    /**
     * @brief Draw a 3D near-surface shell of the SDF with frustum culling
     *
     * Uses compute-shader frustum culling (matching grass path conventions)
     * with CPU fallback.
     *
     * @param camera Camera used to extract frustum planes
     * @param cullRadiusMultiplier Sphere inflation multiplier for culling
     */
    void DrawDebugVolume(const Camera3D &camera, float cullRadiusMultiplier = 1.15f) const;

    /**
     * @brief Draw wireframe outline of the SDF grid volume
     * @param color Wireframe color
     */
    void DrawDebugBounds(Color color) const;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Set voxel grid resolution per axis (clamped to 32..512)
     *
     * Must be called **before** BuildSDF().  Default is 128.
     *
     * @param resolution Number of voxels on each axis
     */
    void SetResolution(int resolution);

    /**
     * @brief Get current grid resolution
     */
    int GetResolution() const { return gridResolution; }

    /**
     * @brief Set the margin (in world units) added around the scene bounds
     *
     * Default is 4.0.
     *
     * @param margin Extra padding on each side of the bounding box
     */
    void SetBoundsMargin(float margin);

    /**
     * @brief Check whether the SDF has been built and is ready for queries
     */
    bool IsReady() const { return sdfReady; }

    /**
     * @brief Get world-space origin of the SDF grid (corner of voxel 0,0,0)
     */
    Vector3 GetGridOrigin() const { return gridOrigin; }

    /**
     * @brief Get world-space size of one voxel
     */
    float GetVoxelSize() const { return voxelSize; }

    /**
     * @brief Get the world-space extents of the SDF grid volume
     */
    Vector3 GetGridExtent() const
    {
        float s = voxelSize * static_cast<float>(gridResolution);
        return {s, s, s};
    }

    /**
     * @brief Enable / disable the collision system entirely
     */
    void SetEnabled(bool enabled) { this->enabled = enabled; }

    /**
     * @brief Check if the collision system is enabled
     */
    bool IsEnabled() const { return enabled; }

private:
    // -----------------------------------------------------------------------
    // GPU-side triangle data (packed for std430)
    // -----------------------------------------------------------------------
    struct GPUTriangle
    {
        float v0[4]; // xyz + pad
        float v1[4]; // xyz + pad
        float v2[4]; // xyz + pad
        float n[4];  // face normal xyz + pad
    };

    // GPU-side entity data (packed for std430)
    struct GPUCollisionEntity
    {
        float positionRadius[4]; // xyz = position, w = radius
    };

    // GPU-side collision result (packed for std430)
    struct GPUCollisionResultData
    {
        float pushVector[4];    // xyz = push * penetration, w = penetration
        float surfaceNormal[4]; // xyz = normal, w = 1 if colliding
    };

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Extract world-space triangles from a mesh, applying a transform
     */
    void ExtractTriangles(const Mesh &mesh, Matrix transform,
                          std::vector<GPUTriangle> &outTriangles) const;

    /**
     * @brief Build a world-space transform matrix from object data
     */
    Matrix BuildObjectTransform(const LevelData::ObjectData &obj) const;

    /**
     * @brief Sample the CPU-side SDF cache at a world position (trilinear)
     *
     * @return Interpolated signed distance; returns +1e4 for out-of-bounds.
     */
    float SampleSDF(Vector3 worldPos) const;

    /**
     * @brief Compute the SDF gradient at a world position via central differences
     */
    Vector3 SDFGradient(Vector3 worldPos) const;

    /**
     * @brief Frustum extraction (same plane convention as grass renderer)
     */
    Frustum ExtractFrustum(const Camera3D &camera) const;

    /**
     * @brief Sphere-vs-frustum test (far + left/right/top/bottom, near skipped)
     */
    bool IsPointInFrustum(const Frustum &frustum, Vector3 point, float radius) const;

    /**
     * @brief Debug rendering sampling step based on grid resolution
     */
    int GetDebugStep() const;

    struct DebugVisibleVoxel
    {
        float x, y, z, dist;
    };

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    bool initialized; ///< Init() has been called
    bool sdfReady;    ///< BuildSDF() completed successfully
    bool enabled;     ///< Runtime toggle

    // Grid parameters
    int gridResolution; ///< Voxels per axis (default 128)
    float boundsMargin; ///< Extra padding around scene AABB (default 4.0)
    Vector3 gridOrigin; ///< World-space corner of voxel (0,0,0)
    float voxelSize;    ///< World-space size of one voxel

    // CPU-side SDF cache (gridResolution^3 floats, X-major layout)
    std::vector<float> sdfCPU;

    // GPU resources – SDF generation
    unsigned int generateProgram; ///< Compute shader program for SDF generation
    unsigned int ssboTriangles;   ///< SSBO holding world-space triangles
    unsigned int sdfTexture3D;    ///< 3D texture (R16F) holding the SDF

    // GPU resources – collision query
    unsigned int collisionProgram; ///< Compute shader program for collision queries
    unsigned int ssboEntities;     ///< SSBO holding entity position+radius
    unsigned int ssboResults;      ///< SSBO holding collision results
    unsigned int sdfSampler;       ///< Sampler object for trilinear filtering

    // GPU resources – debug volume frustum culling
    unsigned int debugCullProgram;                          ///< Compute shader for debug voxel frustum culling
    unsigned int ssboDebugVisible;                          ///< SSBO: [counter+padding][DebugVisibleVoxel...]
    mutable unsigned int debugMaxVisible;                   ///< Maximum visible entries allocated
    mutable std::vector<DebugVisibleVoxel> debugVisibleCPU; ///< Readback cache
};