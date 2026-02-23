#include "DebugRenderer.h"
#include "LightRenderer.h"

DebugRenderer::DebugRenderer()
{
}

DebugRenderer::~DebugRenderer()
{
    Shutdown();
}

void DebugRenderer::Init()
{
    TraceLog(LOG_INFO, "DebugRenderer: Initialized");
}

void DebugRenderer::Shutdown()
{
    TraceLog(LOG_INFO, "DebugRenderer: Shut down");
}

void DebugRenderer::DrawLightDebug(const std::vector<Light> &lights)
{
    for (const auto &light : lights)
    {
        if (light.type == 0)
            continue;

        Vector4 color = light.color;
        DrawSphere({light.positionRadius.x, light.positionRadius.y, light.positionRadius.z},
                   0.3f,
                   {(unsigned char)(color.x * 255),
                    (unsigned char)(color.y * 255),
                    (unsigned char)(color.z * 255),
                    (unsigned char)(color.w * 255)});
    }
}

void DebugRenderer::DrawGrid(int slices, float spacing)
{
    ::DrawGrid(slices, spacing);
}

void DebugRenderer::DrawGeometryStats(int visible, int total, bool gpuCullingEnabled)
{
    // Stats drawn by the caller in DrawUI
    // This method is here for completeness but actual drawing happens in Game::DrawUI
}

void DebugRenderer::DrawGrassStats(int visible, int total)
{
    // Stats drawn by the caller in DrawUI
    // This method is here for completeness but actual drawing happens in Game::DrawUI
}
