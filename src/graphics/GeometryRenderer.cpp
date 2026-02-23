#include "GeometryRenderer.h"
#include "ShaderUtils.h"
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cfloat>
#include <cmath>

GeometryRenderer::GeometryRenderer()
    : computeProgram(0), ssboAllInstances(0), ssboVisibleIndices(0),
      gpuCullingEnabled(true), visibleCount(0), totalInstanceCount(0),
      cullRadiusMultiplier(1.25f)
{
}

GeometryRenderer::~GeometryRenderer()
{
    Shutdown();
}

void GeometryRenderer::Init()
{
    TraceLog(LOG_INFO, "GeometryRenderer: Initializing GPU culling system...");

    computeProgram = CompileComputeProgram("assets/shader/geometry_cull.comp");
    if (computeProgram == 0)
    {
        TraceLog(LOG_WARNING, "GeometryRenderer: Failed to compile compute shader, falling back to CPU culling");
        gpuCullingEnabled = false;
        return;
    }

    // Create SSBOs
    glGenBuffers(1, &ssboAllInstances);
    glGenBuffers(1, &ssboVisibleIndices);

    const int initialCapacity = 1000;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboAllInstances);
    glBufferData(GL_SHADER_STORAGE_BUFFER, initialCapacity * sizeof(ModelInstance), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVisibleIndices);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 16 + initialCapacity * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    TraceLog(LOG_INFO, "GeometryRenderer: GPU culling initialized successfully");
}

void GeometryRenderer::Shutdown()
{
    if (computeProgram != 0)
    {
        glDeleteProgram(computeProgram);
        computeProgram = 0;
    }

    if (ssboAllInstances != 0)
    {
        glDeleteBuffers(1, &ssboAllInstances);
        ssboAllInstances = 0;
    }

    if (ssboVisibleIndices != 0)
    {
        glDeleteBuffers(1, &ssboVisibleIndices);
        ssboVisibleIndices = 0;
    }

    registeredModels.clear();
    modelNameToID.clear();
    allInstances.clear();
    visibleIndices.clear();
}

int GeometryRenderer::RegisterModel(const std::string &name, Model *model)
{
    auto it = modelNameToID.find(name);
    if (it != modelNameToID.end())
        return it->second;

    BoundingBox bounds = GetMeshBoundingBox(model->meshes[0]);

    int modelID = static_cast<int>(registeredModels.size());
    registeredModels.push_back({model, name, bounds});
    modelNameToID[name] = modelID;

    TraceLog(LOG_INFO, "GeometryRenderer: Registered model '%s' with ID %d", name.c_str(), modelID);
    return modelID;
}

void GeometryRenderer::AddInstance(int modelID, Vector3 position, float scale, Vector3 rotation)
{
    if (modelID < 0 || modelID >= static_cast<int>(registeredModels.size()))
    {
        TraceLog(LOG_WARNING, "GeometryRenderer: Invalid modelID %d", modelID);
        return;
    }

    const RegisteredModel &regModel = registeredModels[modelID];

    ModelInstance inst;
    inst.posX = position.x;
    inst.posY = position.y;
    inst.posZ = position.z;
    inst.scale = scale;

    inst.rotX = rotation.x;
    inst.rotY = rotation.y;
    inst.rotZ = rotation.z;
    inst.pad0 = 0.0f;

    inst.boundsMinX = regModel.bounds.min.x;
    inst.boundsMinY = regModel.bounds.min.y;
    inst.boundsMinZ = regModel.bounds.min.z;
    inst.pad1 = 0.0f;

    inst.boundsMaxX = regModel.bounds.max.x;
    inst.boundsMaxY = regModel.bounds.max.y;
    inst.boundsMaxZ = regModel.bounds.max.z;
    inst.pad2 = 0.0f;

    inst.modelID = static_cast<unsigned int>(modelID);
    inst.pad3 = 0;
    inst.pad4 = 0;
    inst.pad5 = 0;

    allInstances.push_back(inst);
}

void GeometryRenderer::ClearInstances()
{
    allInstances.clear();
    visibleIndices.clear();
    visibleCount = 0;
    totalInstanceCount = 0;
}

void GeometryRenderer::Update(float deltaTime, Camera3D camera)
{
    totalInstanceCount = static_cast<int>(allInstances.size());

    if (totalInstanceCount == 0)
    {
        visibleCount = 0;
        return;
    }

    if (gpuCullingEnabled && computeProgram != 0)
    {
        RunGPUCulling(camera);
    }
    else
    {
        RunCPUCulling(camera);
    }
}

void GeometryRenderer::Draw(Camera3D camera)
{
    if (visibleCount == 0)
        return;

    for (int i = 0; i < visibleCount; i++)
    {
        unsigned int idx = (gpuCullingEnabled && computeProgram != 0) ? visibleIndices[i] : visibleIndices[i];

        if (idx >= allInstances.size())
            continue;

        const ModelInstance &inst = allInstances[idx];
        int modelID = static_cast<int>(inst.modelID);

        if (modelID < 0 || modelID >= static_cast<int>(registeredModels.size()))
            continue;

        Model *model = registeredModels[modelID].model;
        Vector3 position = {inst.posX, inst.posY, inst.posZ};
        Vector3 rotation = {inst.rotX, inst.rotY, inst.rotZ};

        if (rotation.x != 0.0f || rotation.y != 0.0f || rotation.z != 0.0f)
        {
            rlPushMatrix();
            rlTranslatef(position.x, position.y, position.z);
            rlRotatef(rotation.x, 1.0f, 0.0f, 0.0f); // pitch (X axis)
            rlRotatef(rotation.y, 0.0f, 1.0f, 0.0f); // yaw (Y axis)
            rlRotatef(rotation.z, 0.0f, 0.0f, 1.0f); // roll (Z axis)
            rlScalef(inst.scale, inst.scale, inst.scale);
            DrawModel(*model, {0, 0, 0}, 1.0f, WHITE);
            rlPopMatrix();
        }
        else
        {
            DrawModel(*model, position, inst.scale, WHITE);
        }
    }
}

Frustum GeometryRenderer::ExtractFrustum(Camera3D camera)
{
    if (!IsGlobalFrustumAvailable())
    {
        UpdateGlobalFrustum(camera);
    }
    return GetGlobalFrustum();
}

bool GeometryRenderer::IsAABBInFrustum(const Frustum &frustum, BoundingBox worldAABB)
{
    Vector3 center = {
        (worldAABB.min.x + worldAABB.max.x) * 0.5f,
        (worldAABB.min.y + worldAABB.max.y) * 0.5f,
        (worldAABB.min.z + worldAABB.max.z) * 0.5f};

    Vector3 extents = {
        (worldAABB.max.x - worldAABB.min.x) * 0.5f,
        (worldAABB.max.y - worldAABB.min.y) * 0.5f,
        (worldAABB.max.z - worldAABB.min.z) * 0.5f};

    float radius = sqrtf(extents.x * extents.x + extents.y * extents.y + extents.z * extents.z) * cullRadiusMultiplier;

    for (int i = 0; i < 6; i++)
    {
        if (i == 4)
            continue;
        const Vector3 &normal = frustum.planes[i].normal;
        float dist = frustum.planes[i].distance;
        float planeDist = normal.x * center.x + normal.y * center.y + normal.z * center.z + dist;
        if (planeDist < -radius)
            return false;
    }

    return true;
}

void GeometryRenderer::RunGPUCulling(Camera3D camera)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboAllInstances);
    glBufferData(GL_SHADER_STORAGE_BUFFER, allInstances.size() * sizeof(ModelInstance),
                 allInstances.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVisibleIndices);
    unsigned int zero[4] = {0, 0, 0, 0};
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 16, zero);

    glBufferData(GL_SHADER_STORAGE_BUFFER, 16 + allInstances.size() * sizeof(unsigned int),
                 nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 16, zero);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    Frustum frustum = ExtractFrustum(camera);

    glUseProgram(computeProgram);

    float frustumData[24]; // 6 planes * 4 floats
    for (int i = 0; i < 6; i++)
    {
        frustumData[i * 4 + 0] = frustum.planes[i].normal.x;
        frustumData[i * 4 + 1] = frustum.planes[i].normal.y;
        frustumData[i * 4 + 2] = frustum.planes[i].normal.z;
        frustumData[i * 4 + 3] = frustum.planes[i].distance;
    }

    int frustumLoc = glGetUniformLocation(computeProgram, "frustumPlanes");
    glUniform4fv(frustumLoc, 6, frustumData);

    int countLoc = glGetUniformLocation(computeProgram, "instanceCount");
    glUniform1ui(countLoc, static_cast<unsigned int>(allInstances.size()));

    int radiusMultLoc = glGetUniformLocation(computeProgram, "radiusMultiplier");
    if (radiusMultLoc >= 0)
    {
        glUniform1f(radiusMultLoc, cullRadiusMultiplier);
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboAllInstances);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboVisibleIndices);

    unsigned int numWorkGroups = (static_cast<unsigned int>(allInstances.size()) + 255) / 256;
    glDispatchCompute(numWorkGroups, 1, 1);

    // Memory barrier
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVisibleIndices);
    unsigned int counter;
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned int), &counter);

    visibleCount = static_cast<int>(counter);

    if (visibleCount > 0)
    {
        visibleIndices.resize(visibleCount);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 16, visibleCount * sizeof(unsigned int),
                           visibleIndices.data());
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUseProgram(0);
}

void GeometryRenderer::RunCPUCulling(Camera3D camera)
{
    Frustum frustum = ExtractFrustum(camera);
    visibleIndices.clear();

    for (size_t i = 0; i < allInstances.size(); i++)
    {
        const ModelInstance &inst = allInstances[i];

        Vector3 worldPos = {inst.posX, inst.posY, inst.posZ};
        Vector3 rotation = {inst.rotX, inst.rotY, inst.rotZ};
        float scale = inst.scale;

        BoundingBox worldAABB;

        if (rotation.x != 0.0f || rotation.y != 0.0f || rotation.z != 0.0f)
        {
            Vector3 localMin = {inst.boundsMinX * scale, inst.boundsMinY * scale, inst.boundsMinZ * scale};
            Vector3 localMax = {inst.boundsMaxX * scale, inst.boundsMaxY * scale, inst.boundsMaxZ * scale};

            Vector3 corners[8] = {
                {localMin.x, localMin.y, localMin.z},
                {localMax.x, localMin.y, localMin.z},
                {localMin.x, localMax.y, localMin.z},
                {localMax.x, localMax.y, localMin.z},
                {localMin.x, localMin.y, localMax.z},
                {localMax.x, localMin.y, localMax.z},
                {localMin.x, localMax.y, localMax.z},
                {localMax.x, localMax.y, localMax.z}};

            // Transform all corners by rotation and find new AABB
            Vector3 transformedMin = {FLT_MAX, FLT_MAX, FLT_MAX};
            Vector3 transformedMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

            for (int c = 0; c < 8; c++)
            {
                // Create rotation matrix
                Matrix matRotation = MatrixRotateXYZ({rotation.x * DEG2RAD,
                                                      rotation.y * DEG2RAD,
                                                      rotation.z * DEG2RAD});

                // Transform corner
                Vector3 transformed = Vector3Transform(corners[c], matRotation);

                // Update min/max
                transformedMin.x = fminf(transformedMin.x, transformed.x);
                transformedMin.y = fminf(transformedMin.y, transformed.y);
                transformedMin.z = fminf(transformedMin.z, transformed.z);
                transformedMax.x = fmaxf(transformedMax.x, transformed.x);
                transformedMax.y = fmaxf(transformedMax.y, transformed.y);
                transformedMax.z = fmaxf(transformedMax.z, transformed.z);
            }

            worldAABB.min = Vector3Add(worldPos, transformedMin);
            worldAABB.max = Vector3Add(worldPos, transformedMax);
        }
        else
        {
            worldAABB.min = {
                worldPos.x + inst.boundsMinX * scale,
                worldPos.y + inst.boundsMinY * scale,
                worldPos.z + inst.boundsMinZ * scale};
            worldAABB.max = {
                worldPos.x + inst.boundsMaxX * scale,
                worldPos.y + inst.boundsMaxY * scale,
                worldPos.z + inst.boundsMaxZ * scale};
        }

        if (IsAABBInFrustum(frustum, worldAABB))
        {
            visibleIndices.push_back(static_cast<unsigned int>(i));
        }
    }

    visibleCount = static_cast<int>(visibleIndices.size());
}
