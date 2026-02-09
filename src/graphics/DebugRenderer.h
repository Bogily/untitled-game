#pragma once

#include "raylib.h"
#include <vector>

// Forward declare Light struct (defined in LightRenderer.h but not part of class)
struct Light;

// Handles all debug visualization: light spheres, grid, stats, etc.
class DebugRenderer
{
public:
    DebugRenderer();
    ~DebugRenderer();

    void Init();
    void Shutdown();

    // Drawing methods
    void DrawLightDebug(const std::vector<Light> &lights);
    void DrawGrid(int slices, float spacing);
    void DrawGeometryStats(int visible, int total, bool gpuCullingEnabled);
    void DrawGrassStats(int visible, int total);

private:
};
