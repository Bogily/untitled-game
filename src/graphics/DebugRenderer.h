/**
 * @file DebugRenderer.h
 * @brief Debug visualization renderer
 */

#pragma once

#include "raylib.h"
#include <vector>

struct Light;

/**
 * @brief Debug visualization for lights, grids, and performance stats
 *
 * Renders debug overlays including light positions, world grid,
 * and rendering statistics.
 */
class DebugRenderer
{
public:
    /**
     * @brief Construct debug renderer
     */
    DebugRenderer();

    /**
     * @brief Destroy debug renderer
     */
    ~DebugRenderer();

    /**
     * @brief Initialize debug renderer resources
     */
    void Init();

    /**
     * @brief Shutdown and cleanup resources
     */
    void Shutdown();

    /**
     * @brief Draw light position spheres and info
     * @param lights Light list to visualize
     */
    void DrawLightDebug(const std::vector<Light> &lights);

    /**
     * @brief Draw world grid
     * @param slices Number of grid divisions
     * @param spacing Distance between grid lines
     */
    void DrawGrid(int slices, float spacing);

    /**
     * @brief Draw geometry culling statistics
     * @param visible Number of visible instances
     * @param total Total instance count
     * @param gpuCullingEnabled Whether GPU culling is active
     */
    void DrawGeometryStats(int visible, int total, bool gpuCullingEnabled);

    /**
     * @brief Draw grass rendering statistics
     * @param visible Number of visible grass instances
     * @param total Total grass count
     */
    void DrawGrassStats(int visible, int total);

private:
};
