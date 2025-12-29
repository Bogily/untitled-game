#include "GrassRenderer.h"
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstring>

// Cross-platform GPU instancing using raylib's rlgl abstraction layer
// No platform-specific code needed!

GrassRenderer::GrassRenderer()
    : windDirection({1.0f, 0.5f}), windStrength(0.5f), windSpeed(2.0f), currentTime(0.0f), timeLoc(-1), windDirLoc(-1), windStrengthLoc(-1), windSpeedLoc(-1), viewPosLoc(-1), matViewLoc(-1), matProjLoc(-1), lightDirLoc(-1), lightColorLoc(-1), grassColorTopLoc(-1), grassColorBottomLoc(-1), ambientStrengthLoc(-1), visibleCount(0), totalGrassCount(0), areaSize(0.0f), grassBladeMesh({0}), instanceVBO(0), lastUploadedCount(0), updateTimeMs(0.0), drawTimeMs(0.0), computeProgram(0), ssboAllInstances(0), ssboVisibleInstances(0), gpuCullingEnabled(true)
{
}

GrassRenderer::~GrassRenderer()
{
    Shutdown();
}

void GrassRenderer::CreateGrassBladeMesh()
{
    // Create a single grass blade triangle (billboarded in shader)
    // Local space: centered at origin, Y-up
    grassBladeMesh.vertexCount = 3;
    grassBladeMesh.triangleCount = 1;

    grassBladeMesh.vertices = (float *)MemAlloc(3 * 3 * sizeof(float));
    grassBladeMesh.texcoords = (float *)MemAlloc(3 * 2 * sizeof(float));
    grassBladeMesh.normals = (float *)MemAlloc(3 * 3 * sizeof(float));
    grassBladeMesh.colors = (unsigned char *)MemAlloc(3 * 4 * sizeof(unsigned char));

    float width = 0.15f;
    float height = 0.8f;

    // Bottom-left vertex
    grassBladeMesh.vertices[0] = -width * 0.5f; // x
    grassBladeMesh.vertices[1] = 0.0f;          // y
    grassBladeMesh.vertices[2] = 0.0f;          // z
    grassBladeMesh.texcoords[0] = 0.0f;
    grassBladeMesh.texcoords[1] = 0.0f;

    // Bottom-right vertex
    grassBladeMesh.vertices[3] = width * 0.5f; // x
    grassBladeMesh.vertices[4] = 0.0f;         // y
    grassBladeMesh.vertices[5] = 0.0f;         // z
    grassBladeMesh.texcoords[2] = 1.0f;
    grassBladeMesh.texcoords[3] = 0.0f;

    // Top vertex (spiky tip)
    grassBladeMesh.vertices[6] = 0.0f;   // x
    grassBladeMesh.vertices[7] = height; // y
    grassBladeMesh.vertices[8] = 0.0f;   // z
    grassBladeMesh.texcoords[4] = 0.5f;
    grassBladeMesh.texcoords[5] = 1.0f;

    // Normals (facing +Z, will be billboarded in shader)
    for (int i = 0; i < 3; i++)
    {
        grassBladeMesh.normals[i * 3 + 0] = 0.0f;
        grassBladeMesh.normals[i * 3 + 1] = 0.0f;
        grassBladeMesh.normals[i * 3 + 2] = 1.0f;
    }

    // Vertex colors (darker at base, lighter at tip)
    // Bottom-left
    grassBladeMesh.colors[0] = 60;
    grassBladeMesh.colors[1] = 120;
    grassBladeMesh.colors[2] = 40;
    grassBladeMesh.colors[3] = 255;
    // Bottom-right
    grassBladeMesh.colors[4] = 60;
    grassBladeMesh.colors[5] = 120;
    grassBladeMesh.colors[6] = 40;
    grassBladeMesh.colors[7] = 255;
    // Top
    grassBladeMesh.colors[8] = 100;
    grassBladeMesh.colors[9] = 200;
    grassBladeMesh.colors[10] = 80;
    grassBladeMesh.colors[11] = 255;

    // Upload mesh to GPU
    UploadMesh(&grassBladeMesh, false);
}

void GrassRenderer::SetupInstanceBuffer()
{
    // Use rlgl cross-platform functions for instancing
    // Create instance buffer using raylib's rlgl (works on all platforms)
    // Create two instance buffers for double-buffering to avoid GPU sync stalls
    instanceVBOs[0] = rlLoadVertexBuffer(nullptr, totalGrassCount * sizeof(InstanceData), true);
    instanceVBOs[1] = rlLoadVertexBuffer(nullptr, totalGrassCount * sizeof(InstanceData), true);
    instanceVBO = instanceVBOs[0];
    currentVBOIndex = 0;
}

// Helper: compile compute shader from file and return program ID (0 on failure)
static unsigned int CompileComputeProgram(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
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

    // Check compile status
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLchar log[4096];
        GLsizei len = 0;
        glGetShaderInfoLog(shader, sizeof(log), &len, log);
        TraceLog(LOG_WARNING, "Compute shader compile error: %s", log);
        glDeleteShader(shader);
        free(src);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);

    // Check link status
    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar log[4096];
        GLsizei len = 0;
        glGetProgramInfoLog(program, sizeof(log), &len, log);
        TraceLog(LOG_WARNING, "Compute shader link error: %s", log);
        glDeleteProgram(program);
        free(src);
        return 0;
    }

    free(src);
    return program;
}

void GrassRenderer::Init(int grassCount, float size)
{
    areaSize = size;
    totalGrassCount = grassCount;

    // Load instanced grass shader
    grassShader = LoadShader("assets/shader/grass_instanced.vs", "assets/shader/grass_instanced.fs");

    // Get shader uniform locations
    timeLoc = GetShaderLocation(grassShader, "time");
    windDirLoc = GetShaderLocation(grassShader, "windDirection");
    windStrengthLoc = GetShaderLocation(grassShader, "windStrength");
    windSpeedLoc = GetShaderLocation(grassShader, "windSpeed");
    viewPosLoc = GetShaderLocation(grassShader, "viewPos");
    matViewLoc = GetShaderLocation(grassShader, "matView");
    matProjLoc = GetShaderLocation(grassShader, "matProjection");
    lightDirLoc = GetShaderLocation(grassShader, "lightDir");
    lightColorLoc = GetShaderLocation(grassShader, "lightColor");
    grassColorTopLoc = GetShaderLocation(grassShader, "grassColorTop");
    grassColorBottomLoc = GetShaderLocation(grassShader, "grassColorBottom");
    ambientStrengthLoc = GetShaderLocation(grassShader, "ambientStrength");

    // Setup material
    grassMaterial = LoadMaterialDefault();
    grassMaterial.shader = grassShader;

    // Create grass texture
    Image grassImg = GenImageColor(1, 1, GREEN);
    grassMaterial.maps[MATERIAL_MAP_DIFFUSE].texture = LoadTextureFromImage(grassImg);
    UnloadImage(grassImg);

    // Create grass blade mesh
    CreateGrassBladeMesh();

    // Generate grass positions
    GenerateGrassPositions(grassCount, size);

    // Setup GPU culling resources (SSBOs + compute shader)
    if (gpuCullingEnabled)
    {
        // compile compute shader
        computeProgram = CompileComputeProgram("assets/shader/grass_cull.comp");
        if (computeProgram == 0)
        {
            TraceLog(LOG_WARNING, "Failed to compile grass compute shader, falling back to CPU culling");
            gpuCullingEnabled = false;
        }
        else
        {
            // Create SSBO for all instances (input)
            glGenBuffers(1, &ssboAllInstances);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboAllInstances);
            glBufferData(GL_SHADER_STORAGE_BUFFER, totalGrassCount * sizeof(InstanceData), allInstances.data(), GL_STATIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboAllInstances);

            // Create SSBO for visible instances (output). Reserve space for a counter (uint) + padding to 16 bytes + max instances
            // std430 requires array elements to be 16-byte aligned; ensure array starts at offset 16
            size_t visSize = 16 + (size_t)totalGrassCount * sizeof(InstanceData);
            glGenBuffers(1, &ssboVisibleInstances);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVisibleInstances);
            glBufferData(GL_SHADER_STORAGE_BUFFER, visSize, NULL, GL_DYNAMIC_COPY);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboVisibleInstances);

            // Ensure bindings cleared
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
    }

    // Setup OpenGL instance buffer
    SetupInstanceBuffer();

    // Reserve space for visible instances
    visibleInstances.reserve(grassCount);

    TraceLog(LOG_INFO, "GrassRenderer initialized with %d grass instances (GPU Instanced Rendering)", grassCount);
}

void GrassRenderer::Update(float deltaTime, Camera3D camera)
{
    currentTime += deltaTime;
    double t0 = GetTime();

    // Frustum culling - only update instance buffer, no vertex updates!
    Frustum frustum = ExtractFrustum(camera);

    visibleInstances.clear();

    if (gpuCullingEnabled && computeProgram != 0)
    {
        // GPU culling path: dispatch compute shader to compact visible instances into SSBO
        // Reset visible counter (first uint in ssboVisibleInstances)
        unsigned int zero = 0;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVisibleInstances);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned int), &zero);

        // Set compute uniforms (frustum planes, radius, instanceCount)
        glUseProgram(computeProgram);
        // Upload frustum planes as vec4 uniforms
        for (int i = 0; i < 6; ++i)
        {
            char name[64];
            sprintf(name, "frustumPlanes[%d]", i);
            GLint loc = glGetUniformLocation(computeProgram, name);
            if (loc >= 0)
            {
                glUniform4f(loc, frustum.planes[i].normal.x, frustum.planes[i].normal.y, frustum.planes[i].normal.z, frustum.planes[i].distance);
            }
        }
        GLint rloc = glGetUniformLocation(computeProgram, "radius");
        if (rloc >= 0)
            glUniform1f(rloc, 1.5f);
        GLint icloc = glGetUniformLocation(computeProgram, "instanceCount");
        if (icloc >= 0)
            glUniform1ui(icloc, (unsigned int)totalGrassCount);

        // Bind SSBOs to the binding points used in shader
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboAllInstances);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboVisibleInstances);

        // Dispatch compute
        unsigned int groups = (totalGrassCount + 255) / 256;
        glDispatchCompute(groups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        // Read back visible count (first uint in ssboVisibleInstances)
        unsigned int visCount = 0;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVisibleInstances);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned int), &visCount);
        visibleCount = (int)visCount;
        lastUploadedCount = visibleCount; // keep consistent with CPU path
        // done GPU path
    }
    else
    {
        // If we have a spatial grid, do coarse cell culling first
        if (!gridCells.empty())
        {
            float halfCell = gridCellSize * 0.5f;
            float cellRadius = sqrtf(halfCell * halfCell + halfCell * halfCell);

            for (int row = 0; row < gridRows; ++row)
            {
                for (int col = 0; col < gridCols; ++col)
                {
                    int cellIndex = row * gridCols + col;
                    if (gridCells[cellIndex].empty())
                        continue;

                    float cx = gridOriginX + (col + 0.5f) * gridCellSize;
                    float cz = gridOriginZ + (row + 0.5f) * gridCellSize;
                    Vector3 cellCenter = {cx, 0.0f, cz};

                    // coarse test for the whole cell
                    if (!IsPointInFrustum(frustum, cellCenter, cellRadius + 1.5f))
                        continue;

                    // fine test per-instance in cell
                    const std::vector<int> &cellList = gridCells[cellIndex];
                    for (int idx : cellList)
                    {
                        const InstanceData &instance = allInstances[idx];
                        Vector3 pos = {instance.x, instance.y, instance.z};
                        if (IsPointInFrustum(frustum, pos, 1.5f))
                        {
                            visibleInstances.push_back(instance);
                        }
                    }
                }
            }
        }
        else
        {
            for (const auto &instance : allInstances)
            {
                Vector3 pos = {instance.x, instance.y, instance.z};
                if (IsPointInFrustum(frustum, pos, 1.5f))
                {
                    visibleInstances.push_back(instance);
                }
            }
        }

        visibleCount = (int)visibleInstances.size();

        // Update instance buffer on GPU using rlgl (cross-platform)
        if (visibleCount > 0)
        {
            // Avoid redundant uploads when visible count didn't change (common when camera static)
            if (visibleCount != lastUploadedCount)
            {
                // Upload into next VBO to avoid GPU sync with currently used buffer
                currentVBOIndex = (currentVBOIndex + 1) % 2;
                unsigned int uploadVBO = instanceVBOs[currentVBOIndex];
                rlUpdateVertexBuffer(uploadVBO, visibleInstances.data(), visibleCount * sizeof(InstanceData), 0);
                lastUploadedCount = visibleCount;
            }
            // else: skip upload
        }
        else
        {
            lastUploadedCount = 0;
        }

        updateTimeMs = (GetTime() - t0) * 1000.0;
    }
}

void GrassRenderer::Draw(Camera3D camera)
{
    if (visibleCount == 0)
        return;
    double t0 = GetTime();

    // Make sure raylib's batch is flushed before we do custom rendering
    rlDrawRenderBatchActive();

    Matrix matView = rlGetMatrixModelview();
    Matrix matProj = rlGetMatrixProjection();

    // Update shader uniforms
    SetShaderValue(grassShader, timeLoc, &currentTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, windDirLoc, &windDirection, SHADER_UNIFORM_VEC2);
    SetShaderValue(grassShader, windStrengthLoc, &windStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, windSpeedLoc, &windSpeed, SHADER_UNIFORM_FLOAT);

    Vector3 camPos = camera.position;
    SetShaderValue(grassShader, viewPosLoc, &camPos, SHADER_UNIFORM_VEC3);

    SetShaderValueMatrix(grassShader, matViewLoc, matView);
    SetShaderValueMatrix(grassShader, matProjLoc, matProj);

    // Light direction (sun)
    Vector3 lightDir = Vector3Normalize({-0.5f, -1.0f, -0.3f});
    SetShaderValue(grassShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);

    Vector3 lightColor = {1.0f, 0.95f, 0.9f};
    SetShaderValue(grassShader, lightColorLoc, &lightColor, SHADER_UNIFORM_VEC3);

    Vector3 grassColorTop = {0.4f, 0.8f, 0.3f};
    Vector3 grassColorBottom = {0.2f, 0.4f, 0.15f};
    SetShaderValue(grassShader, grassColorTopLoc, &grassColorTop, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassColorBottomLoc, &grassColorBottom, SHADER_UNIFORM_VEC3);

    float ambientStrength = 0.3f;
    SetShaderValue(grassShader, ambientStrengthLoc, &ambientStrength, SHADER_UNIFORM_FLOAT);

    // Set up rendering state for grass
    rlDisableBackfaceCulling(); // Grass visible from both sides
    rlEnableDepthTest();        // Enable depth testing
    rlEnableDepthMask();        // Enable depth writing

    // Enable shader
    rlEnableShader(grassShader.id);

    // Bind mesh VAO
    rlEnableVertexArray(grassBladeMesh.vaoId);

    if (gpuCullingEnabled && ssboVisibleInstances != 0)
    {
        // When using GPU culling, bind the visible SSBO as the instance vertex buffer
        // Buffer layout: [uint counter padding (16 bytes)][vec4 instance0][vec4 instance1]...
        unsigned int activeVBO = ssboVisibleInstances;
        rlEnableVertexBuffer(activeVBO);
        // Attribute offset: skip the 16-byte header (counter + padding)
        int ssboHeaderOffset = 16; // std430 alignment ensures array starts at 16
        rlSetVertexAttribute(4, 4, RL_FLOAT, false, sizeof(InstanceData), ssboHeaderOffset);
        rlEnableVertexAttribute(4);
        rlSetVertexAttributeDivisor(4, 1);
        rlDisableVertexBuffer();

        // Draw using instances written by compute shader
        rlDrawVertexArrayInstanced(0, grassBladeMesh.vertexCount, visibleCount);
    }
    else
    {
        // Bind current instance VBO and set instance attribute before drawing
        unsigned int activeVBO = instanceVBOs[currentVBOIndex];
        rlEnableVertexBuffer(activeVBO);
        rlSetVertexAttribute(4, 4, RL_FLOAT, false, sizeof(InstanceData), 0);
        rlEnableVertexAttribute(4);
        rlSetVertexAttributeDivisor(4, 1);
        rlDisableVertexBuffer();

        // Draw all instances with a single call! (cross-platform)
        rlDrawVertexArrayInstanced(0, grassBladeMesh.vertexCount, visibleCount);
    }

    rlDisableVertexArray();
    rlDisableShader();

    // Restore default state
    rlEnableBackfaceCulling();

    // Force raylib to sync its internal state
    rlDrawRenderBatchActive();

    drawTimeMs = (GetTime() - t0) * 1000.0;
}

void GrassRenderer::Shutdown()
{
    for (int i = 0; i < 2; ++i)
    {
        if (instanceVBOs[i] > 0)
        {
            rlUnloadVertexBuffer(instanceVBOs[i]);
            instanceVBOs[i] = 0;
        }
    }

    if (grassBladeMesh.vertexCount > 0)
    {
        UnloadMesh(grassBladeMesh);
        grassBladeMesh = {0};
    }

    if (grassMaterial.maps[MATERIAL_MAP_DIFFUSE].texture.id > 0)
    {
        UnloadTexture(grassMaterial.maps[MATERIAL_MAP_DIFFUSE].texture);
    }

    if (grassShader.id > 0)
    {
        UnloadShader(grassShader);
    }
    // Delete compute shader and SSBOs
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
    if (ssboVisibleInstances != 0)
    {
        glDeleteBuffers(1, &ssboVisibleInstances);
        ssboVisibleInstances = 0;
    }

    allInstances.clear();
    visibleInstances.clear();
    TraceLog(LOG_INFO, "GrassRenderer shutdown");
}

void GrassRenderer::GenerateGrassPositions(int count, float size)
{
    allInstances.clear();
    allInstances.reserve(count);

    srand(42);
    float halfSize = size / 2.0f;

    for (int i = 0; i < count; i++)
    {
        InstanceData instance;
        instance.x = ((float)rand() / RAND_MAX) * size - halfSize;
        instance.y = 0.0f;
        instance.z = ((float)rand() / RAND_MAX) * size - halfSize;
        instance.scale = 0.8f + ((float)rand() / RAND_MAX) * 0.4f;

        allInstances.push_back(instance);
    }

    // Build spatial grid for faster culling
    gridCellSize = 4.0f; // tweak this value for performance/accuracy
    if (gridCellSize <= 0.0f)
        gridCellSize = 4.0f;

    gridCols = (int)ceil(size / gridCellSize);
    gridRows = (int)ceil(size / gridCellSize);
    if (gridCols < 1)
        gridCols = 1;
    if (gridRows < 1)
        gridRows = 1;

    gridOriginX = -halfSize;
    gridOriginZ = -halfSize;

    gridCells.clear();
    gridCells.resize(gridCols * gridRows);

    for (int i = 0; i < (int)allInstances.size(); ++i)
    {
        const InstanceData &inst = allInstances[i];
        int col = (int)floor((inst.x - gridOriginX) / gridCellSize);
        int row = (int)floor((inst.z - gridOriginZ) / gridCellSize);
        if (col < 0)
            col = 0;
        if (col >= gridCols)
            col = gridCols - 1;
        if (row < 0)
            row = 0;
        if (row >= gridRows)
            row = gridRows - 1;
        int cellIndex = row * gridCols + col;
        gridCells[cellIndex].push_back(i);
    }
}

Frustum GrassRenderer::ExtractFrustum(Camera3D camera)
{
    Frustum frustum;

    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    Matrix matView = MatrixLookAt(camera.position, camera.target, camera.up);
    Matrix matProj = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.1f, 1000.0f);
    Matrix matViewProj = MatrixMultiply(matView, matProj);

    // Left plane
    frustum.planes[0].normal.x = matViewProj.m3 + matViewProj.m0;
    frustum.planes[0].normal.y = matViewProj.m7 + matViewProj.m4;
    frustum.planes[0].normal.z = matViewProj.m11 + matViewProj.m8;
    frustum.planes[0].distance = matViewProj.m15 + matViewProj.m12;

    // Right plane
    frustum.planes[1].normal.x = matViewProj.m3 - matViewProj.m0;
    frustum.planes[1].normal.y = matViewProj.m7 - matViewProj.m4;
    frustum.planes[1].normal.z = matViewProj.m11 - matViewProj.m8;
    frustum.planes[1].distance = matViewProj.m15 - matViewProj.m12;

    // Bottom plane
    frustum.planes[2].normal.x = matViewProj.m3 + matViewProj.m1;
    frustum.planes[2].normal.y = matViewProj.m7 + matViewProj.m5;
    frustum.planes[2].normal.z = matViewProj.m11 + matViewProj.m9;
    frustum.planes[2].distance = matViewProj.m15 + matViewProj.m13;

    // Top plane
    frustum.planes[3].normal.x = matViewProj.m3 - matViewProj.m1;
    frustum.planes[3].normal.y = matViewProj.m7 - matViewProj.m5;
    frustum.planes[3].normal.z = matViewProj.m11 - matViewProj.m9;
    frustum.planes[3].distance = matViewProj.m15 - matViewProj.m13;

    // Near plane
    frustum.planes[4].normal.x = matViewProj.m3 + matViewProj.m2;
    frustum.planes[4].normal.y = matViewProj.m7 + matViewProj.m6;
    frustum.planes[4].normal.z = matViewProj.m11 + matViewProj.m10;
    frustum.planes[4].distance = matViewProj.m15 + matViewProj.m14;

    // Far plane
    frustum.planes[5].normal.x = matViewProj.m3 - matViewProj.m2;
    frustum.planes[5].normal.y = matViewProj.m7 - matViewProj.m6;
    frustum.planes[5].normal.z = matViewProj.m11 - matViewProj.m10;
    frustum.planes[5].distance = matViewProj.m15 - matViewProj.m14;

    // Normalize all planes
    for (int i = 0; i < 6; i++)
    {
        float length = Vector3Length(frustum.planes[i].normal);
        if (length > 0.0f)
        {
            frustum.planes[i].normal = Vector3Scale(frustum.planes[i].normal, 1.0f / length);
            frustum.planes[i].distance /= length;
        }
    }

    return frustum;
}

bool GrassRenderer::IsPointInFrustum(const Frustum &frustum, Vector3 point, float radius)
{
    for (int i = 0; i < 6; i++)
    {
        float distance = Vector3DotProduct(frustum.planes[i].normal, point) + frustum.planes[i].distance;
        if (distance < -radius)
            return false;
    }
    return true;
}
