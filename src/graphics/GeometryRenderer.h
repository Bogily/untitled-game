#pragma once
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "Frustum.h"
#include <vector>
#include <unordered_map>
#include <string>

class GeometryRenderer
{
public:
    GeometryRenderer();
    ~GeometryRenderer();

    void Init();
    void Update(float deltaTime, Camera3D camera);
    void Shutdown();

    // Register a model type and get its ID
    int RegisterModel(const std::string &name, Model *model);

    // Add an instance of a registered model
    void AddInstance(int modelID, Vector3 position, float scale = 1.0f, Vector3 rotation = {0, 0, 0});

    // Clear all instances (call at start of frame if dynamic)
    void ClearInstances();

    // Draw all visible instances after culling
    void Draw(Camera3D camera);

    // Stats
    int GetVisibleCount() const { return visibleCount; }
    int GetTotalCount() const { return totalInstanceCount; }

    // GPU culling toggle
    void SetGPUCullingEnabled(bool enabled) { gpuCullingEnabled = enabled; }
    bool IsGPUCullingEnabled() const { return gpuCullingEnabled; }
    void SetCullingRadiusMultiplier(float m) { cullRadiusMultiplier = m; }
    float GetCullingRadiusMultiplier() const { return cullRadiusMultiplier; }

private:
    struct ModelInstance
    {
        float posX, posY, posZ, scale;
        float rotX, rotY, rotZ, pad0; // Euler angles in degrees
        float boundsMinX, boundsMinY, boundsMinZ, pad1;
        float boundsMaxX, boundsMaxY, boundsMaxZ, pad2;
        unsigned int modelID;
        unsigned int pad3, pad4, pad5; // Padding for 16-byte alignment
    };

    struct RegisteredModel
    {
        Model *model;
        std::string name;
        BoundingBox bounds; // Local-space AABB
    };

    // Registered models
    std::vector<RegisteredModel> registeredModels;
    std::unordered_map<std::string, int> modelNameToID;

    // Instance data
    std::vector<ModelInstance> allInstances;
    std::vector<unsigned int> visibleIndices; // CPU fallback

    // GPU culling resources
    unsigned int computeProgram;
    unsigned int ssboAllInstances;
    unsigned int ssboVisibleIndices;
    bool gpuCullingEnabled;
    float cullRadiusMultiplier; // >1.0 retains objects longer (default 1.1)

    // Stats
    int visibleCount;
    int totalInstanceCount;

    // Frustum culling (CPU fallback)
    Frustum ExtractFrustum(Camera3D camera);
    bool IsAABBInFrustum(const Frustum &frustum, BoundingBox worldAABB);

    // GPU culling
    void RunGPUCulling(Camera3D camera);
    void RunCPUCulling(Camera3D camera);

    // Helper: compile compute shader
    unsigned int CompileComputeProgram(const char *path);
};
