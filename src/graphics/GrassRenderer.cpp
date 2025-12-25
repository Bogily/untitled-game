#include "GrassRenderer.h"
#include <cmath>
#include <cstdlib>
#include <cstring>

// Cross-platform GPU instancing using raylib's rlgl abstraction layer
// No platform-specific code needed!

GrassRenderer::GrassRenderer()
    : windDirection({1.0f, 0.5f})
    , windStrength(0.5f)
    , windSpeed(2.0f)
    , currentTime(0.0f)
    , timeLoc(-1)
    , windDirLoc(-1)
    , windStrengthLoc(-1)
    , windSpeedLoc(-1)
    , viewPosLoc(-1)
    , matViewLoc(-1)
    , matProjLoc(-1)
    , lightDirLoc(-1)
    , lightColorLoc(-1)
    , grassColorTopLoc(-1)
    , grassColorBottomLoc(-1)
    , ambientStrengthLoc(-1)
    , visibleCount(0)
    , totalGrassCount(0)
    , areaSize(0.0f)
    , grassBladeMesh({0})
    , instanceVBO(0)
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
    
    grassBladeMesh.vertices = (float*)MemAlloc(3 * 3 * sizeof(float));
    grassBladeMesh.texcoords = (float*)MemAlloc(3 * 2 * sizeof(float));
    grassBladeMesh.normals = (float*)MemAlloc(3 * 3 * sizeof(float));
    grassBladeMesh.colors = (unsigned char*)MemAlloc(3 * 4 * sizeof(unsigned char));
    
    float width = 0.15f;
    float height = 0.8f;
    
    // Bottom-left vertex
    grassBladeMesh.vertices[0] = -width * 0.5f;  // x
    grassBladeMesh.vertices[1] = 0.0f;           // y
    grassBladeMesh.vertices[2] = 0.0f;           // z
    grassBladeMesh.texcoords[0] = 0.0f;
    grassBladeMesh.texcoords[1] = 0.0f;
    
    // Bottom-right vertex
    grassBladeMesh.vertices[3] = width * 0.5f;   // x
    grassBladeMesh.vertices[4] = 0.0f;           // y
    grassBladeMesh.vertices[5] = 0.0f;           // z
    grassBladeMesh.texcoords[2] = 1.0f;
    grassBladeMesh.texcoords[3] = 0.0f;
    
    // Top vertex (spiky tip)
    grassBladeMesh.vertices[6] = 0.0f;           // x
    grassBladeMesh.vertices[7] = height;         // y
    grassBladeMesh.vertices[8] = 0.0f;           // z
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
    instanceVBO = rlLoadVertexBuffer(nullptr, totalGrassCount * sizeof(InstanceData), true); // dynamic = true
    
    // Get VAO from mesh and set up instance attribute
    rlEnableVertexArray(grassBladeMesh.vaoId);
    
    // Bind our instance buffer and set up attribute
    rlEnableVertexBuffer(instanceVBO);
    
    // Instance attribute at location 4 (vec4: xyz = position, w = scale)
    rlSetVertexAttribute(4, 4, RL_FLOAT, false, sizeof(InstanceData), 0);
    rlEnableVertexAttribute(4);
    rlSetVertexAttributeDivisor(4, 1);  // This is the key! One per instance, not per vertex
    
    rlDisableVertexBuffer();
    rlDisableVertexArray();
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
    
    // Setup OpenGL instance buffer
    SetupInstanceBuffer();
    
    // Reserve space for visible instances
    visibleInstances.reserve(grassCount);
    
    TraceLog(LOG_INFO, "GrassRenderer initialized with %d grass instances (GPU Instanced Rendering)", grassCount);
}

void GrassRenderer::Update(float deltaTime, Camera3D camera)
{
    currentTime += deltaTime;
    
    // Frustum culling - only update instance buffer, no vertex updates!
    Frustum frustum = ExtractFrustum(camera);
    
    visibleInstances.clear();
    
    for (const auto& instance : allInstances)
    {
        Vector3 pos = {instance.x, instance.y, instance.z};
        if (IsPointInFrustum(frustum, pos, 1.5f))
        {
            visibleInstances.push_back(instance);
        }
    }
    
    visibleCount = (int)visibleInstances.size();
    
    // Update instance buffer on GPU using rlgl (cross-platform)
    if (visibleCount > 0)
    {
        rlUpdateVertexBuffer(instanceVBO, visibleInstances.data(), visibleCount * sizeof(InstanceData), 0);
    }
}

void GrassRenderer::Draw(Camera3D camera)
{
    if (visibleCount == 0) return;
    
    // Make sure raylib's batch is flushed before we do custom rendering
    rlDrawRenderBatchActive();
    
    // Get the ACTUAL matrices from rlgl (same as what raylib uses for other objects)
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
    rlDisableBackfaceCulling();     // Grass visible from both sides
    rlEnableDepthTest();            // Enable depth testing
    rlEnableDepthMask();            // Enable depth writing
    
    // Enable shader
    rlEnableShader(grassShader.id);
    
    // Bind mesh VAO (with instance attribute already set up)
    rlEnableVertexArray(grassBladeMesh.vaoId);
    
    // Draw all instances with a single call! (cross-platform)
    rlDrawVertexArrayInstanced(0, grassBladeMesh.vertexCount, visibleCount);
    
    rlDisableVertexArray();
    rlDisableShader();
    
    // Restore default state
    rlEnableBackfaceCulling();
    
    // Force raylib to sync its internal state
    rlDrawRenderBatchActive();
}

void GrassRenderer::Shutdown()
{
    if (instanceVBO > 0)
    {
        rlUnloadVertexBuffer(instanceVBO);
        instanceVBO = 0;
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

bool GrassRenderer::IsPointInFrustum(const Frustum& frustum, Vector3 point, float radius)
{
    for (int i = 0; i < 6; i++)
    {
        float distance = Vector3DotProduct(frustum.planes[i].normal, point) + frustum.planes[i].distance;
        if (distance < -radius)
            return false;
    }
    return true;
}
