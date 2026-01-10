#include "GeometryRenderer.h"
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

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

    // Compile compute shader
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

    // Allocate initial space (will grow dynamically)
    const int initialCapacity = 1000;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboAllInstances);
    glBufferData(GL_SHADER_STORAGE_BUFFER, initialCapacity * sizeof(ModelInstance), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVisibleIndices);
    // counter (uint) + padding (3 uints) + indices array
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
    // Check if already registered
    auto it = modelNameToID.find(name);
    if (it != modelNameToID.end())
        return it->second;

    // Calculate bounding box
    BoundingBox bounds = GetMeshBoundingBox(model->meshes[0]);

    // Register new model
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

    // Run culling
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

    // Draw each visible instance
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

        // Check if rotation is needed
        if (rotation.x != 0.0f || rotation.y != 0.0f || rotation.z != 0.0f)
        {
            // Apply transformation manually for rotated instances
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
            // Simple draw for non-rotated instances
            DrawModel(*model, position, inst.scale, WHITE);
        }
    }
}

Frustum GeometryRenderer::ExtractFrustum(Camera3D camera)
{
    Frustum frustum;
    Matrix viewProj = MatrixMultiply(GetCameraMatrix(camera),
                                     MatrixPerspective(camera.fovy * DEG2RAD,
                                                       (float)GetScreenWidth() / (float)GetScreenHeight(),
                                                       0.1, 1000.0));

    // Extract frustum planes from view-projection matrix
    // Left plane
    frustum.planes[0].normal = {viewProj.m3 + viewProj.m0, viewProj.m7 + viewProj.m4, viewProj.m11 + viewProj.m8};
    frustum.planes[0].distance = viewProj.m15 + viewProj.m12;

    // Right plane
    frustum.planes[1].normal = {viewProj.m3 - viewProj.m0, viewProj.m7 - viewProj.m4, viewProj.m11 - viewProj.m8};
    frustum.planes[1].distance = viewProj.m15 - viewProj.m12;

    // Bottom plane
    frustum.planes[2].normal = {viewProj.m3 + viewProj.m1, viewProj.m7 + viewProj.m5, viewProj.m11 + viewProj.m9};
    frustum.planes[2].distance = viewProj.m15 + viewProj.m13;

    // Top plane
    frustum.planes[3].normal = {viewProj.m3 - viewProj.m1, viewProj.m7 - viewProj.m5, viewProj.m11 - viewProj.m9};
    frustum.planes[3].distance = viewProj.m15 - viewProj.m13;

    // Near plane
    frustum.planes[4].normal = {viewProj.m3 + viewProj.m2, viewProj.m7 + viewProj.m6, viewProj.m11 + viewProj.m10};
    frustum.planes[4].distance = viewProj.m15 + viewProj.m14;

    // Far plane
    frustum.planes[5].normal = {viewProj.m3 - viewProj.m2, viewProj.m7 - viewProj.m6, viewProj.m11 - viewProj.m10};
    frustum.planes[5].distance = viewProj.m15 - viewProj.m14;

    // Normalize planes
    for (int i = 0; i < 6; i++)
    {
        float length = sqrtf(frustum.planes[i].normal.x * frustum.planes[i].normal.x +
                             frustum.planes[i].normal.y * frustum.planes[i].normal.y +
                             frustum.planes[i].normal.z * frustum.planes[i].normal.z);
        frustum.planes[i].normal.x /= length;
        frustum.planes[i].normal.y /= length;
        frustum.planes[i].normal.z /= length;
        frustum.planes[i].distance /= length;
    }

    // Push near plane back to prevent early culling
    frustum.planes[4].distance -= 5.0f;

    return frustum;
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

    // Conservative: test bounding sphere against planes
    float radius = sqrtf(extents.x * extents.x + extents.y * extents.y + extents.z * extents.z) * cullRadiusMultiplier;

    for (int i = 0; i < 6; i++)
    {
        // Skip near plane for stability when objects are very close
        if (i == 4)
            continue;
        const Vector3 &normal = frustum.planes[i].normal;
        float dist = frustum.planes[i].distance;
        float planeDist = normal.x * center.x + normal.y * center.y + normal.z * center.z + dist;
        if (planeDist < -radius)
            return false; // fully outside
    }

    return true; // intersects or inside
}

void GeometryRenderer::RunGPUCulling(Camera3D camera)
{
    // Upload instance data to GPU
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboAllInstances);
    glBufferData(GL_SHADER_STORAGE_BUFFER, allInstances.size() * sizeof(ModelInstance),
                 allInstances.data(), GL_DYNAMIC_DRAW);

    // Reset counter in visible buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVisibleIndices);
    unsigned int zero[4] = {0, 0, 0, 0};
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 16, zero);

    // Ensure buffer is large enough
    glBufferData(GL_SHADER_STORAGE_BUFFER, 16 + allInstances.size() * sizeof(unsigned int),
                 nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 16, zero);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Extract frustum planes
    Frustum frustum = ExtractFrustum(camera);

    // Run compute shader
    glUseProgram(computeProgram);

    // Upload frustum planes
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

    // Bind SSBOs
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboAllInstances);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboVisibleIndices);

    // Dispatch compute shader
    unsigned int numWorkGroups = (static_cast<unsigned int>(allInstances.size()) + 255) / 256;
    glDispatchCompute(numWorkGroups, 1, 1);

    // Memory barrier
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Read back visible count
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVisibleIndices);
    unsigned int counter;
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned int), &counter);

    visibleCount = static_cast<int>(counter);

    // Read back visible indices
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

        // Calculate world-space AABB
        Vector3 worldPos = {inst.posX, inst.posY, inst.posZ};
        float scale = inst.scale;

        BoundingBox worldAABB;
        worldAABB.min = {
            worldPos.x + inst.boundsMinX * scale,
            worldPos.y + inst.boundsMinY * scale,
            worldPos.z + inst.boundsMinZ * scale};
        worldAABB.max = {
            worldPos.x + inst.boundsMaxX * scale,
            worldPos.y + inst.boundsMaxY * scale,
            worldPos.z + inst.boundsMaxZ * scale};

        if (IsAABBInFrustum(frustum, worldAABB))
        {
            visibleIndices.push_back(static_cast<unsigned int>(i));
        }
    }

    visibleCount = static_cast<int>(visibleIndices.size());
}

unsigned int GeometryRenderer::CompileComputeProgram(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        TraceLog(LOG_WARNING, "GeometryRenderer: Cannot open compute shader: %s", path);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *src = (char *)malloc(size + 1);
    if (!src)
    {
        fclose(f);
        return 0;
    }

    fread(src, 1, size, f);
    src[size] = '\0';
    fclose(f);

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, (const char **)&src, NULL);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLchar log[4096];
        GLsizei len = 0;
        glGetShaderInfoLog(shader, sizeof(log), &len, log);
        TraceLog(LOG_WARNING, "GeometryRenderer: Compute shader compile error: %s", log);
        glDeleteShader(shader);
        free(src);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar log[4096];
        GLsizei len = 0;
        glGetProgramInfoLog(program, sizeof(log), &len, log);
        TraceLog(LOG_WARNING, "GeometryRenderer: Compute program link error: %s", log);
        glDeleteProgram(program);
        free(src);
        return 0;
    }

    free(src);
    TraceLog(LOG_INFO, "GeometryRenderer: Compute shader compiled successfully: %s", path);
    return program;
}
