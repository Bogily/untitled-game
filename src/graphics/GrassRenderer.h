#pragma once
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>

// Frustum plane structure for culling
struct FrustumPlane
{
    Vector3 normal;
    float distance;
};

struct Frustum
{
    FrustumPlane planes[6]; // Near, Far, Left, Right, Top, Bottom
};

class GrassRenderer
{
public:
    GrassRenderer();
    ~GrassRenderer();

    void Init(int grassCount, float areaSize);
    void Update(float deltaTime, Camera3D camera);
    void Draw(Camera3D camera);
    void Shutdown();

    // Stats
    int GetVisibleCount() const { return visibleCount; }
    int GetTotalCount() const { return totalGrassCount; }

    // Customization
    void SetWindDirection(Vector2 direction) { windDirection = direction; }
    void SetWindStrength(float strength) { windStrength = strength; }
    void SetWindSpeed(float speed) { windSpeed = speed; }

private:
    // Instance data: xyz = position, w = scale
    struct InstanceData
    {
        float x, y, z;
        float scale;
    };

    Shader grassShader;
    Mesh grassBladeMesh; // Single grass blade geometry (reused for all instances)
    Material grassMaterial;

    // Instance buffers
    std::vector<InstanceData> allInstances;     // All grass positions
    std::vector<InstanceData> visibleInstances; // Visible after frustum culling
    unsigned int instanceVBO;                   // legacy single VBO (kept for compatibility)
    unsigned int instanceVBOs[2];               // double-buffered VBOs to reduce GPU stalls
    int currentVBOIndex;

    // Shader locations
    int timeLoc;
    int windDirLoc;
    int windStrengthLoc;
    int windSpeedLoc;
    int viewPosLoc;
    int matViewLoc;
    int matProjLoc;
    int lightDirLoc;
    int lightColorLoc;
    int grassColorTopLoc;
    int grassColorBottomLoc;
    int ambientStrengthLoc;

    // Wind parameters
    Vector2 windDirection;
    float windStrength;
    float windSpeed;
    float currentTime;

    // Stats
    int visibleCount;
    int totalGrassCount;
    float areaSize;

    // Setup functions
    void CreateGrassBladeMesh();
    void GenerateGrassPositions(int count, float size);
    void SetupInstanceBuffer();

    // Frustum culling
    Frustum ExtractFrustum(Camera3D camera);
    bool IsPointInFrustum(const Frustum &frustum, Vector3 point, float radius);
    // Spatial grid for faster CPU culling
    int gridCols;
    int gridRows;
    float gridCellSize;
    float gridOriginX;
    float gridOriginZ;
    std::vector<std::vector<int>> gridCells; // cell -> indices into allInstances
    void BuildSpatialGrid();
    // Upload tracking and simple profiling
    int lastUploadedCount;
    double updateTimeMs;
    double drawTimeMs;
    // GPU culling resources
    unsigned int computeProgram;
    unsigned int ssboAllInstances;
    unsigned int ssboVisibleInstances;
    bool gpuCullingEnabled;
};
