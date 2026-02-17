/**
 * @file SDFCollisionSystem.cpp
 * @brief GPU-accelerated SDF collision system implementation
 */

#include "SDFCollisionSystem.h"
#include "../graphics/ShaderUtils.h"
#include "rlgl.h"
#include <glad/glad.h>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <cstdint>
// ---------------------------------------------------------------------------
// Helpers – keep uniform scale extraction consistent with the rest of the code
// ---------------------------------------------------------------------------
namespace
{
    float AverageScale(Vector3 s)
    {
        return (s.x + s.y + s.z) / 3.0f;
    }

    int ClampInt(int v, int lo, int hi)
    {
        if (v < lo)
            return lo;
        if (v > hi)
            return hi;
        return v;
    }

    float Clamp01(float v)
    {
        if (v < 0.0f)
            return 0.0f;
        if (v > 1.0f)
            return 1.0f;
        return v;
    }

} // anonymous namespace

namespace
{
    constexpr uint32_t SDF_CACHE_MAGIC = 0x53444643; // 'SDFC'
    constexpr uint32_t SDF_CACHE_VERSION = 1;

    struct SDFCacheHeader
    {
        uint32_t magic;
        uint32_t version;
        uint32_t resolution;
        uint32_t reserved;
        float gridOrigin[3];
        float voxelSize;
        uint64_t voxelCount;
    };
}

// ===========================================================================
// Construction / Destruction
// ===========================================================================

SDFCollisionSystem::SDFCollisionSystem()
    : initialized(false),
      sdfReady(false),
      enabled(true),
      gridResolution(128),
      boundsMargin(4.0f),
      gridOrigin{0.0f, 0.0f, 0.0f},
      voxelSize(0.5f),
      generateProgram(0),
      ssboTriangles(0),
      sdfTexture3D(0),
      collisionProgram(0),
      ssboEntities(0),
      ssboResults(0),
      sdfSampler(0),
      debugCullProgram(0),
      ssboDebugVisible(0),
      debugMaxVisible(0),
      debugCubeShader{0},
      debugCubeMesh{0},
      debugInstanceVBO(0),
      debugInstanceCapacity(0),
      debugMatViewLoc(-1),
      debugMatProjLoc(-1),
      debugCubeScaleLoc(-1),
      debugShellThresholdLoc(-1),
      sdfCacheEnabled(true)
{
}

SDFCollisionSystem::~SDFCollisionSystem()
{
    Shutdown();
}

// ===========================================================================
// Lifecycle
// ===========================================================================

void SDFCollisionSystem::Init()
{
    if (initialized)
        return;

    TraceLog(LOG_INFO, "SDFCollisionSystem: Initializing...");

    // --- Compile compute shaders -------------------------------------------
    generateProgram = CompileComputeProgram("assets/shader/sdf_generate.comp");
    if (generateProgram == 0)
    {
        TraceLog(LOG_ERROR, "SDFCollisionSystem: Failed to compile SDF generation shader");
        return;
    }

    collisionProgram = CompileComputeProgram("assets/shader/sdf_collision.comp");
    if (collisionProgram == 0)
    {
        TraceLog(LOG_ERROR, "SDFCollisionSystem: Failed to compile collision query shader");
        glDeleteProgram(generateProgram);
        generateProgram = 0;
        return;
    }

    debugCullProgram = CompileComputeProgram("assets/shader/sdf_debug_cull.comp");
    if (debugCullProgram == 0)
    {
        TraceLog(LOG_WARNING, "SDFCollisionSystem: Failed to compile debug culling shader (CPU fallback active)");
    }

    // --- Create SSBOs (initial empty allocation) ---------------------------
    glGenBuffers(1, &ssboTriangles);
    glGenBuffers(1, &ssboEntities);
    glGenBuffers(1, &ssboResults);
    glGenBuffers(1, &ssboDebugVisible);

    // --- Create sampler for trilinear SDF reads ----------------------------
    glGenSamplers(1, &sdfSampler);
    glSamplerParameteri(sdfSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(sdfSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(sdfSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sdfSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sdfSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    debugCubeShader = LoadShader("assets/shader/colored_cube_instanced.vs",
                                 "assets/shader/colored_cube_instanced.fs");
    if (debugCubeShader.id > 0)
    {
        debugMatViewLoc = GetShaderLocation(debugCubeShader, "matView");
        debugMatProjLoc = GetShaderLocation(debugCubeShader, "matProjection");
        debugCubeScaleLoc = GetShaderLocation(debugCubeShader, "cubeScale");
        debugShellThresholdLoc = GetShaderLocation(debugCubeShader, "shellThreshold");
    }
    else
    {
        TraceLog(LOG_WARNING, "SDFCollisionSystem: Failed to load colored_cube_instanced shader (fallback draw path active)");
    }

    debugCubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    UploadMesh(&debugCubeMesh, false);

    debugInstanceCapacity = 4096;
    debugInstanceVBO = rlLoadVertexBuffer(nullptr,
                                          debugInstanceCapacity * static_cast<int>(sizeof(DebugVisibleVoxel)),
                                          true);

    initialized = true;
    TraceLog(LOG_INFO, "SDFCollisionSystem: Initialized successfully");
}

void SDFCollisionSystem::Shutdown()
{
    if (generateProgram)
    {
        glDeleteProgram(generateProgram);
        generateProgram = 0;
    }
    if (collisionProgram)
    {
        glDeleteProgram(collisionProgram);
        collisionProgram = 0;
    }
    if (debugCullProgram)
    {
        glDeleteProgram(debugCullProgram);
        debugCullProgram = 0;
    }
    if (ssboTriangles)
    {
        glDeleteBuffers(1, &ssboTriangles);
        ssboTriangles = 0;
    }
    if (ssboEntities)
    {
        glDeleteBuffers(1, &ssboEntities);
        ssboEntities = 0;
    }
    if (ssboResults)
    {
        glDeleteBuffers(1, &ssboResults);
        ssboResults = 0;
    }
    if (ssboDebugVisible)
    {
        glDeleteBuffers(1, &ssboDebugVisible);
        ssboDebugVisible = 0;
    }
    if (sdfTexture3D)
    {
        glDeleteTextures(1, &sdfTexture3D);
        sdfTexture3D = 0;
    }
    if (sdfSampler)
    {
        glDeleteSamplers(1, &sdfSampler);
        sdfSampler = 0;
    }
    if (debugInstanceVBO > 0)
    {
        rlUnloadVertexBuffer(debugInstanceVBO);
        debugInstanceVBO = 0;
    }
    if (debugCubeMesh.vertexCount > 0)
    {
        UnloadMesh(debugCubeMesh);
        debugCubeMesh = {0};
    }
    if (debugCubeShader.id > 0)
    {
        UnloadShader(debugCubeShader);
        debugCubeShader = {0};
    }

    sdfCPU.clear();
    debugVisibleCPU.clear();
    debugMaxVisible = 0;
    debugInstanceCapacity = 0;
    sdfReady = false;
    initialized = false;

    TraceLog(LOG_INFO, "SDFCollisionSystem: Shutdown complete");
}

// ===========================================================================
// Configuration
// ===========================================================================

void SDFCollisionSystem::SetResolution(int resolution)
{
    gridResolution = (resolution < 32) ? 32 : resolution; // below 32 the SDF is too coarse to be useful
}

int SDFCollisionSystem::GetDebugStep() const
{
    if (gridResolution >= 256)
        return 4;
    if (gridResolution >= 192)
        return 3;
    if (gridResolution > 128)
        return 2;
    return 1;
}
void SDFCollisionSystem::SetBoundsMargin(float margin)
{
    boundsMargin = fmaxf(0.0f, margin);
}

// ===========================================================================
// Triangle extraction helpers
// ===========================================================================

Matrix SDFCollisionSystem::BuildObjectTransform(const LevelData::ObjectData &obj) const
{
    // Match the rendering transform order used by GeometryRenderer::Draw():
    //   translate -> rotateX -> rotateY -> rotateZ -> scale(uniform)
    float uniformScale = AverageScale(obj.scale);

    Matrix matScale = MatrixScale(uniformScale, uniformScale, uniformScale);
    Matrix matRotation = MatrixRotateXYZ({obj.rotation.x * DEG2RAD,
                                          obj.rotation.y * DEG2RAD,
                                          obj.rotation.z * DEG2RAD});
    Matrix matTranslate = MatrixTranslate(obj.position.x, obj.position.y, obj.position.z);

    // Order: scale first, then rotate, then translate  (column-major: T * R * S)
    return MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslate);
}

void SDFCollisionSystem::ExtractTriangles(const Mesh &mesh, Matrix transform,
                                          std::vector<GPUTriangle> &outTriangles) const
{
    if (!mesh.vertices)
        return;

    // Compute the normal-transform matrix (inverse-transpose of upper-left 3x3).
    // For uniform scale + rotation this is simply the rotation part, but we
    // handle the general case for correctness.
    Matrix normalMatrix = MatrixTranspose(MatrixInvert(transform));

    int triCount = mesh.triangleCount;

    for (int t = 0; t < triCount; t++)
    {
        unsigned int i0, i1, i2;

        if (mesh.indices)
        {
            i0 = mesh.indices[t * 3 + 0];
            i1 = mesh.indices[t * 3 + 1];
            i2 = mesh.indices[t * 3 + 2];
        }
        else
        {
            i0 = static_cast<unsigned int>(t * 3 + 0);
            i1 = static_cast<unsigned int>(t * 3 + 1);
            i2 = static_cast<unsigned int>(t * 3 + 2);
        }

        // Local-space vertex positions
        Vector3 lv0 = {mesh.vertices[i0 * 3 + 0], mesh.vertices[i0 * 3 + 1], mesh.vertices[i0 * 3 + 2]};
        Vector3 lv1 = {mesh.vertices[i1 * 3 + 0], mesh.vertices[i1 * 3 + 1], mesh.vertices[i1 * 3 + 2]};
        Vector3 lv2 = {mesh.vertices[i2 * 3 + 0], mesh.vertices[i2 * 3 + 1], mesh.vertices[i2 * 3 + 2]};

        // World-space positions
        Vector3 wv0 = Vector3Transform(lv0, transform);
        Vector3 wv1 = Vector3Transform(lv1, transform);
        Vector3 wv2 = Vector3Transform(lv2, transform);

        // Compute face normal from world-space edges
        Vector3 edge1 = Vector3Subtract(wv1, wv0);
        Vector3 edge2 = Vector3Subtract(wv2, wv0);
        Vector3 faceN = Vector3CrossProduct(edge1, edge2);
        float faceNLen = Vector3Length(faceN);

        if (faceNLen > 1e-10f)
        {
            faceN = Vector3Scale(faceN, 1.0f / faceNLen);
        }
        else
        {
            // Degenerate triangle – use vertex normal if available, else skip
            if (mesh.normals)
            {
                faceN = {mesh.normals[i0 * 3 + 0], mesh.normals[i0 * 3 + 1], mesh.normals[i0 * 3 + 2]};
                faceN = Vector3Normalize(Vector3Transform(faceN, normalMatrix));
            }
            else
            {
                faceN = {0.0f, 1.0f, 0.0f}; // fallback up
            }
        }

        GPUTriangle tri;
        tri.v0[0] = wv0.x;
        tri.v0[1] = wv0.y;
        tri.v0[2] = wv0.z;
        tri.v0[3] = 0.0f;
        tri.v1[0] = wv1.x;
        tri.v1[1] = wv1.y;
        tri.v1[2] = wv1.z;
        tri.v1[3] = 0.0f;
        tri.v2[0] = wv2.x;
        tri.v2[1] = wv2.y;
        tri.v2[2] = wv2.z;
        tri.v2[3] = 0.0f;
        tri.n[0] = faceN.x;
        tri.n[1] = faceN.y;
        tri.n[2] = faceN.z;
        tri.n[3] = 0.0f;

        outTriangles.push_back(tri);
    }
}

// ===========================================================================
// SDF construction
// ===========================================================================

void SDFCollisionSystem::BuildSDF(const std::vector<LevelData::ObjectData> &objects,
                                  const std::unordered_map<std::string, Model> &sceneModels)
{
    if (!initialized)
    {
        TraceLog(LOG_ERROR, "SDFCollisionSystem: Cannot BuildSDF – not initialized");
        return;
    }

    sdfReady = false;

    // ------------------------------------------------------------------
    // 1. Extract world-space triangles from every static object
    // ------------------------------------------------------------------
    std::vector<GPUTriangle> allTriangles;
    allTriangles.reserve(8192); // rough pre-alloc

    // Track world bounds while extracting
    Vector3 boundsMin = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3 boundsMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (const auto &obj : objects)
    {
        // Only process static objects for the collision SDF
        if (obj.mobility != LevelData::ObjectData::Mobility::Static)
            continue;

        auto modelIt = sceneModels.find(obj.modelType);
        if (modelIt == sceneModels.end())
            continue;

        const Model &model = modelIt->second;
        Matrix xform = BuildObjectTransform(obj);

        for (int m = 0; m < model.meshCount; m++)
        {
            size_t before = allTriangles.size();
            ExtractTriangles(model.meshes[m], xform, allTriangles);

            // Update bounds from newly added triangles
            for (size_t ti = before; ti < allTriangles.size(); ti++)
            {
                for (int vi = 0; vi < 3; vi++)
                {
                    const float *v = (vi == 0)   ? allTriangles[ti].v0
                                     : (vi == 1) ? allTriangles[ti].v1
                                                 : allTriangles[ti].v2;
                    boundsMin.x = fminf(boundsMin.x, v[0]);
                    boundsMin.y = fminf(boundsMin.y, v[1]);
                    boundsMin.z = fminf(boundsMin.z, v[2]);
                    boundsMax.x = fmaxf(boundsMax.x, v[0]);
                    boundsMax.y = fmaxf(boundsMax.y, v[1]);
                    boundsMax.z = fmaxf(boundsMax.z, v[2]);
                }
            }
        }
    }

    if (allTriangles.empty())
    {
        TraceLog(LOG_WARNING, "SDFCollisionSystem: No static triangles found – SDF will be empty");
        // Create a trivial SDF with large positive values (no geometry)
        boundsMin = {-10.0f, -2.0f, -10.0f};
        boundsMax = {10.0f, 10.0f, 10.0f};
    }

    // ------------------------------------------------------------------
    // 2. Compute grid parameters
    // ------------------------------------------------------------------
    // Add margin around the scene bounds
    boundsMin.x -= boundsMargin;
    boundsMin.y -= boundsMargin;
    boundsMin.z -= boundsMargin;
    boundsMax.x += boundsMargin;
    boundsMax.y += boundsMargin;
    boundsMax.z += boundsMargin;

    // Compute voxel size so the longest axis fits in gridResolution voxels.
    // All three axes use the same voxel size (cubic voxels) but may have
    // fewer voxels along shorter axes.  For simplicity we use a uniform
    // cubic grid whose side length equals the longest axis of the AABB.
    float extentX = boundsMax.x - boundsMin.x;
    float extentY = boundsMax.y - boundsMin.y;
    float extentZ = boundsMax.z - boundsMin.z;
    float maxExtent = fmaxf(extentX, fmaxf(extentY, extentZ));

    // Ensure maxExtent is positive
    if (maxExtent < 1.0f)
        maxExtent = 1.0f;

    voxelSize = maxExtent / static_cast<float>(gridResolution);
    gridOrigin = boundsMin;

    const uint64_t cacheKey = ComputeSceneHash(allTriangles);
    if (sdfCacheEnabled && LoadSDFCache(cacheKey))
    {
        sdfReady = true;
        TraceLog(LOG_INFO, "SDFCollisionSystem: Loaded SDF cache (res=%d)", gridResolution);
        return;
    }

    TraceLog(LOG_INFO, "SDFCollisionSystem: Building SDF from %d scene objects...",
             static_cast<int>(objects.size()));
    TraceLog(LOG_INFO, "SDFCollisionSystem: Extracted %d triangles, bounds [%.1f,%.1f,%.1f]-[%.1f,%.1f,%.1f]",
             static_cast<int>(allTriangles.size()),
             boundsMin.x, boundsMin.y, boundsMin.z,
             boundsMax.x, boundsMax.y, boundsMax.z);
    TraceLog(LOG_INFO, "SDFCollisionSystem: Grid %d^3, voxelSize=%.4f, origin=[%.2f,%.2f,%.2f]",
             gridResolution, voxelSize, gridOrigin.x, gridOrigin.y, gridOrigin.z);

    // ------------------------------------------------------------------
    // 3. Upload triangles to GPU SSBO
    // ------------------------------------------------------------------
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboTriangles);
    if (!allTriangles.empty())
    {
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     static_cast<GLsizeiptr>(allTriangles.size() * sizeof(GPUTriangle)),
                     allTriangles.data(), GL_STATIC_DRAW);
    }
    else
    {
        // Upload a dummy triangle so the SSBO binding is valid
        GPUTriangle dummy{};
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GPUTriangle), &dummy, GL_STATIC_DRAW);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // ------------------------------------------------------------------
    // 4. Create / recreate the 3D SDF texture
    // ------------------------------------------------------------------
    if (sdfTexture3D)
        glDeleteTextures(1, &sdfTexture3D);

    glGenTextures(1, &sdfTexture3D);
    glBindTexture(GL_TEXTURE_3D, sdfTexture3D);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R16F,
                 gridResolution, gridResolution, gridResolution,
                 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_3D, 0);

    // ------------------------------------------------------------------
    // 5. Dispatch the SDF generation compute shader
    // ------------------------------------------------------------------
    glUseProgram(generateProgram);

    // Bind triangle SSBO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboTriangles);

    // Bind 3D texture as image for writing
    glBindImageTexture(0, sdfTexture3D, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R16F);

    // Set uniforms
    GLint locTriCount = glGetUniformLocation(generateProgram, "triangleCount");
    GLint locGridOrigin = glGetUniformLocation(generateProgram, "gridOrigin");
    GLint locVoxelSize = glGetUniformLocation(generateProgram, "voxelSize");
    GLint locGridSize = glGetUniformLocation(generateProgram, "gridSize");

    glUniform1ui(locTriCount, static_cast<unsigned int>(allTriangles.size()));
    glUniform3f(locGridOrigin, gridOrigin.x, gridOrigin.y, gridOrigin.z);
    glUniform1f(locVoxelSize, voxelSize);
    glUniform3i(locGridSize, gridResolution, gridResolution, gridResolution);

    // Dispatch in chunks to avoid long single-dispatch GPU stalls (TDR risk)
    GLint locChunkOffset = glGetUniformLocation(generateProgram, "chunkOffset");
    GLint locChunkSize = glGetUniformLocation(generateProgram, "chunkSize");

    const int chunkDim = 64; // 64^3 chunk
    const unsigned int groupsPerChunkAxis = (static_cast<unsigned int>(chunkDim) + 3u) / 4u;
    int chunkDispatches = 0;

    for (int z0 = 0; z0 < gridResolution; z0 += chunkDim)
    {
        for (int y0 = 0; y0 < gridResolution; y0 += chunkDim)
        {
            for (int x0 = 0; x0 < gridResolution; x0 += chunkDim)
            {
                int sx = std::min(chunkDim, gridResolution - x0);
                int sy = std::min(chunkDim, gridResolution - y0);
                int sz = std::min(chunkDim, gridResolution - z0);

                glUniform3i(locChunkOffset, x0, y0, z0);
                glUniform3i(locChunkSize, sx, sy, sz);

                unsigned int gx = (static_cast<unsigned int>(sx) + 3u) / 4u;
                unsigned int gy = (static_cast<unsigned int>(sy) + 3u) / 4u;
                unsigned int gz = (static_cast<unsigned int>(sz) + 3u) / 4u;

                glDispatchCompute(gx, gy, gz);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                chunkDispatches++;
                if ((chunkDispatches % 8) == 0)
                    glFlush();
            }
        }
    }

    // Ensure writes are visible before readback
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glUseProgram(0);

    TraceLog(LOG_INFO, "SDFCollisionSystem: GPU SDF generation dispatched in %d chunks (chunkDim=%d)",
             chunkDispatches, chunkDim);

    // ------------------------------------------------------------------
    // 6. Read back the SDF texture to CPU for fast single queries
    // ------------------------------------------------------------------
    {
        size_t totalVoxels = static_cast<size_t>(gridResolution) *
                             static_cast<size_t>(gridResolution) *
                             static_cast<size_t>(gridResolution);
        sdfCPU.resize(totalVoxels);

        glBindTexture(GL_TEXTURE_3D, sdfTexture3D);

        // GL_R16F stores half-floats; we read back as GL_FLOAT and the driver converts.
        glGetTexImage(GL_TEXTURE_3D, 0, GL_RED, GL_FLOAT, sdfCPU.data());

        glBindTexture(GL_TEXTURE_3D, 0);

        TraceLog(LOG_INFO, "SDFCollisionSystem: CPU cache populated (%zu voxels, %.2f MB)",
                 totalVoxels, static_cast<float>(totalVoxels * sizeof(float)) / (1024.0f * 1024.0f));
    }

    if (sdfCacheEnabled)
        SaveSDFCache(cacheKey);

    sdfReady = true;
    TraceLog(LOG_INFO, "SDFCollisionSystem: SDF build complete");
}

uint64_t SDFCollisionSystem::ComputeSceneHash(const std::vector<GPUTriangle> &triangles) const
{
    uint64_t hash = 1469598103934665603ull;
    auto mixBytes = [&](const void *ptr, size_t len)
    {
        const unsigned char *p = static_cast<const unsigned char *>(ptr);
        for (size_t i = 0; i < len; ++i)
        {
            hash ^= static_cast<uint64_t>(p[i]);
            hash *= 1099511628211ull;
        }
    };

    mixBytes(&gridResolution, sizeof(gridResolution));
    mixBytes(&boundsMargin, sizeof(boundsMargin));
    mixBytes(&gridOrigin, sizeof(gridOrigin));
    mixBytes(&voxelSize, sizeof(voxelSize));

    uint64_t triCount = static_cast<uint64_t>(triangles.size());
    mixBytes(&triCount, sizeof(triCount));
    if (!triangles.empty())
        mixBytes(triangles.data(), triangles.size() * sizeof(GPUTriangle));

    return hash;
}

std::string SDFCollisionSystem::GetCacheFilePath(uint64_t cacheKey) const
{
    char fileName[64];
    std::snprintf(fileName, sizeof(fileName), "sdf_%016llx.bin", static_cast<unsigned long long>(cacheKey));
    std::filesystem::path p = std::filesystem::path("assets") / "cache" / "sdf" / fileName;
    return p.string();
}

bool SDFCollisionSystem::UploadSDFTextureFromCPU()
{
    if (sdfCPU.empty() || gridResolution <= 0)
        return false;

    if (sdfTexture3D)
        glDeleteTextures(1, &sdfTexture3D);

    glGenTextures(1, &sdfTexture3D);
    glBindTexture(GL_TEXTURE_3D, sdfTexture3D);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R16F,
                 gridResolution, gridResolution, gridResolution,
                 0, GL_RED, GL_FLOAT, sdfCPU.data());
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_3D, 0);

    return true;
}

bool SDFCollisionSystem::LoadSDFCache(uint64_t cacheKey)
{
    const std::string cachePath = GetCacheFilePath(cacheKey);
    std::ifstream file(cachePath, std::ios::binary);
    if (!file.is_open())
        return false;

    SDFCacheHeader header{};
    file.read(reinterpret_cast<char *>(&header), sizeof(header));
    if (!file.good())
        return false;

    if (header.magic != SDF_CACHE_MAGIC || header.version != SDF_CACHE_VERSION)
        return false;

    const uint64_t expectedVoxelCount = static_cast<uint64_t>(header.resolution) *
                                        static_cast<uint64_t>(header.resolution) *
                                        static_cast<uint64_t>(header.resolution);
    if (header.voxelCount != expectedVoxelCount)
        return false;

    sdfCPU.resize(static_cast<size_t>(header.voxelCount));
    file.read(reinterpret_cast<char *>(sdfCPU.data()), static_cast<std::streamsize>(header.voxelCount * sizeof(float)));
    if (!file.good())
        return false;

    gridResolution = static_cast<int>(header.resolution);
    gridOrigin = {header.gridOrigin[0], header.gridOrigin[1], header.gridOrigin[2]};
    voxelSize = header.voxelSize;

    return UploadSDFTextureFromCPU();
}

void SDFCollisionSystem::SaveSDFCache(uint64_t cacheKey) const
{
    if (sdfCPU.empty() || gridResolution <= 0)
        return;

    const std::string cachePath = GetCacheFilePath(cacheKey);
    std::filesystem::path pathObj(cachePath);
    std::error_code ec;
    std::filesystem::create_directories(pathObj.parent_path(), ec);

    std::ofstream file(cachePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
        return;

    SDFCacheHeader header{};
    header.magic = SDF_CACHE_MAGIC;
    header.version = SDF_CACHE_VERSION;
    header.resolution = static_cast<uint32_t>(gridResolution);
    header.gridOrigin[0] = gridOrigin.x;
    header.gridOrigin[1] = gridOrigin.y;
    header.gridOrigin[2] = gridOrigin.z;
    header.voxelSize = voxelSize;
    header.voxelCount = static_cast<uint64_t>(sdfCPU.size());

    file.write(reinterpret_cast<const char *>(&header), sizeof(header));
    file.write(reinterpret_cast<const char *>(sdfCPU.data()),
               static_cast<std::streamsize>(sdfCPU.size() * sizeof(float)));
}

// ===========================================================================
// CPU-side SDF sampling (trilinear interpolation)
// ===========================================================================

float SDFCollisionSystem::SampleSDF(Vector3 worldPos) const
{
    if (!sdfReady || sdfCPU.empty())
        return 1.0e4f;

    const float gridExtent = voxelSize * static_cast<float>(gridResolution);
    if (worldPos.x < gridOrigin.x || worldPos.x > gridOrigin.x + gridExtent ||
        worldPos.y < gridOrigin.y || worldPos.y > gridOrigin.y + gridExtent ||
        worldPos.z < gridOrigin.z || worldPos.z > gridOrigin.z + gridExtent)
    {
        return 1.0e4f;
    }

    // Convert world position to continuous voxel coordinates
    float fx = (worldPos.x - gridOrigin.x) / voxelSize - 0.5f;
    float fy = (worldPos.y - gridOrigin.y) / voxelSize - 0.5f;
    float fz = (worldPos.z - gridOrigin.z) / voxelSize - 0.5f;

    // Integer voxel coordinates for the 8 neighbors
    int ix = static_cast<int>(floorf(fx));
    int iy = static_cast<int>(floorf(fy));
    int iz = static_cast<int>(floorf(fz));

    // Fractional part
    float tx = fx - static_cast<float>(ix);
    float ty = fy - static_cast<float>(iy);
    float tz = fz - static_cast<float>(iz);

    // Clamp to grid
    int maxIdx = gridResolution - 1;

    auto sample = [&](int x, int y, int z) -> float
    {
        x = ClampInt(x, 0, maxIdx);
        y = ClampInt(y, 0, maxIdx);
        z = ClampInt(z, 0, maxIdx);
        size_t index = static_cast<size_t>(x) + static_cast<size_t>(y) * static_cast<size_t>(gridResolution) + static_cast<size_t>(z) * static_cast<size_t>(gridResolution) * static_cast<size_t>(gridResolution);
        return sdfCPU[index];
    };

    // Trilinear interpolation
    float c000 = sample(ix, iy, iz);
    float c100 = sample(ix + 1, iy, iz);
    float c010 = sample(ix, iy + 1, iz);
    float c110 = sample(ix + 1, iy + 1, iz);
    float c001 = sample(ix, iy, iz + 1);
    float c101 = sample(ix + 1, iy, iz + 1);
    float c011 = sample(ix, iy + 1, iz + 1);
    float c111 = sample(ix + 1, iy + 1, iz + 1);

    float c00 = c000 * (1.0f - tx) + c100 * tx;
    float c10 = c010 * (1.0f - tx) + c110 * tx;
    float c01 = c001 * (1.0f - tx) + c101 * tx;
    float c11 = c011 * (1.0f - tx) + c111 * tx;

    float c0 = c00 * (1.0f - ty) + c10 * ty;
    float c1 = c01 * (1.0f - ty) + c11 * ty;

    return c0 * (1.0f - tz) + c1 * tz;
}

Vector3 SDFCollisionSystem::SDFGradient(Vector3 worldPos) const
{
    float eps = voxelSize * 0.5f;

    float dx = SampleSDF({worldPos.x + eps, worldPos.y, worldPos.z}) - SampleSDF({worldPos.x - eps, worldPos.y, worldPos.z});
    float dy = SampleSDF({worldPos.x, worldPos.y + eps, worldPos.z}) - SampleSDF({worldPos.x, worldPos.y - eps, worldPos.z});
    float dz = SampleSDF({worldPos.x, worldPos.y, worldPos.z + eps}) - SampleSDF({worldPos.x, worldPos.y, worldPos.z - eps});

    Vector3 g = {dx, dy, dz};
    float len = Vector3Length(g);

    if (len < 1e-8f)
        return {0.0f, 1.0f, 0.0f}; // Degenerate; default to "push up"

    return Vector3Scale(g, 1.0f / len);
}

Frustum SDFCollisionSystem::ExtractFrustum(const Camera3D &camera) const
{
    if (!IsGlobalFrustumAvailable())
    {
        UpdateGlobalFrustum(camera);
    }
    return GetGlobalFrustum();
}

bool SDFCollisionSystem::IsPointInFrustum(const Frustum &frustum, Vector3 point, float radius) const
{
    if (Vector3DotProduct(frustum.planes[5].normal, point) + frustum.planes[5].distance < -radius)
        return false;

    for (int i = 0; i < 4; i++)
    {
        float distance = Vector3DotProduct(frustum.planes[i].normal, point) + frustum.planes[i].distance;
        if (distance < -radius)
            return false;
    }

    return true;
}

void SDFCollisionSystem::EnsureDebugInstanceCapacity(int count) const
{
    if (count <= debugInstanceCapacity)
        return;

    int newCapacity = debugInstanceCapacity > 0 ? debugInstanceCapacity : 1024;
    while (newCapacity < count)
        newCapacity *= 2;

    if (debugInstanceVBO > 0)
        rlUnloadVertexBuffer(debugInstanceVBO);

    debugInstanceVBO = rlLoadVertexBuffer(nullptr,
                                          newCapacity * static_cast<int>(sizeof(DebugVisibleVoxel)),
                                          true);
    debugInstanceCapacity = newCapacity;
}

void SDFCollisionSystem::DrawDebugVoxelsInstanced(const DebugVisibleVoxel *voxels,
                                                  int count,
                                                  float shellThreshold,
                                                  float cubeSize) const
{
    if (count <= 0 || voxels == nullptr)
        return;

    if (debugCubeShader.id <= 0 || debugCubeMesh.vaoId == 0 || debugInstanceVBO == 0)
    {
        for (int i = 0; i < count; ++i)
        {
            float absDist = fabsf(voxels[i].dist);
            float intensity = Clamp01(1.0f - absDist / shellThreshold);
            unsigned char alpha = static_cast<unsigned char>(90.0f + intensity * 165.0f);
            Color color = (voxels[i].dist < 0.0f)
                              ? Color{255, 70, 70, alpha}
                              : Color{80, 180, 255, alpha};
            DrawCube({voxels[i].x, voxels[i].y, voxels[i].z}, cubeSize, cubeSize, cubeSize, color);
        }
        return;
    }

    EnsureDebugInstanceCapacity(count);
    rlUpdateVertexBuffer(debugInstanceVBO,
                         voxels,
                         count * static_cast<int>(sizeof(DebugVisibleVoxel)),
                         0);

    rlDrawRenderBatchActive();

    Matrix matView = rlGetMatrixModelview();
    Matrix matProj = rlGetMatrixProjection();

    if (debugMatViewLoc >= 0)
        SetShaderValueMatrix(debugCubeShader, debugMatViewLoc, matView);
    if (debugMatProjLoc >= 0)
        SetShaderValueMatrix(debugCubeShader, debugMatProjLoc, matProj);
    if (debugCubeScaleLoc >= 0)
        SetShaderValue(debugCubeShader, debugCubeScaleLoc, &cubeSize, SHADER_UNIFORM_FLOAT);
    if (debugShellThresholdLoc >= 0)
        SetShaderValue(debugCubeShader, debugShellThresholdLoc, &shellThreshold, SHADER_UNIFORM_FLOAT);

    rlEnableDepthTest();
    rlEnableDepthMask();
    rlEnableShader(debugCubeShader.id);
    rlEnableVertexArray(debugCubeMesh.vaoId);

    rlEnableVertexBuffer(debugInstanceVBO);
    rlSetVertexAttribute(4, 4, RL_FLOAT, false, sizeof(DebugVisibleVoxel), 0);
    rlEnableVertexAttribute(4);
    rlSetVertexAttributeDivisor(4, 1);
    rlDisableVertexBuffer();

    if (debugCubeMesh.indices != nullptr)
        rlDrawVertexArrayElementsInstanced(0, debugCubeMesh.triangleCount * 3, 0, count);
    else
        rlDrawVertexArrayInstanced(0, debugCubeMesh.vertexCount, count);

    rlDisableVertexArray();
    rlDisableShader();
    rlDrawRenderBatchActive();
}
// ===========================================================================
// Collision queries – single entity (CPU path)
// ===========================================================================

SDFCollisionResult SDFCollisionSystem::QueryCollision(Vector3 position, float radius) const
{
    SDFCollisionResult result;
    result.pushVector = {0.0f, 0.0f, 0.0f};
    result.surfaceNormal = {0.0f, 1.0f, 0.0f};
    result.penetrationDepth = 0.0f;
    result.colliding = false;

    if (!sdfReady || !enabled)
        return result;

    float dist = SampleSDF(position);

    float absDist = fabsf(dist);
    if (absDist < radius)
    {
        Vector3 normal = SDFGradient(position);
        if (dist < 0.0f)
            normal = Vector3Scale(normal, -1.0f);

        float penetration = radius - absDist;

        result.pushVector = Vector3Scale(normal, penetration);
        result.surfaceNormal = normal;
        result.penetrationDepth = penetration;
        result.colliding = true;
    }

    return result;
}

void SDFCollisionSystem::DrawDebugVolume(const Camera3D &camera, float cullRadiusMultiplier) const
{
    if (!sdfReady || sdfCPU.empty())
        return;

    const int step = GetDebugStep();
    const float shellThreshold = voxelSize * static_cast<float>(step) * 1.2f;
    const float cubeSize = voxelSize * static_cast<float>(step) * 0.55f;
    const float voxelCullRadius = cubeSize * 0.5f;
    const Frustum frustum = ExtractFrustum(camera);

    // Ensure debug visible buffer matches current stepped voxel count
    const unsigned int steppedRes = (static_cast<unsigned int>(gridResolution) + static_cast<unsigned int>(step) - 1u) / static_cast<unsigned int>(step);
    const unsigned int totalCandidates = steppedRes * steppedRes * steppedRes;

    if (ssboDebugVisible != 0 && debugMaxVisible != totalCandidates)
    {
        const size_t headerSize = 16;
        const size_t bodySize = static_cast<size_t>(totalCandidates) * sizeof(DebugVisibleVoxel);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboDebugVisible);
        glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(headerSize + bodySize), nullptr, GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        debugMaxVisible = totalCandidates;
        debugVisibleCPU.resize(debugMaxVisible);
    }

    if (debugCullProgram != 0 && ssboDebugVisible != 0 && debugMaxVisible > 0)
    {
        unsigned int zero = 0;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboDebugVisible);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned int), &zero);

        glUseProgram(debugCullProgram);

        for (int i = 0; i < 6; ++i)
        {
            char name[64];
            sprintf(name, "frustumPlanes[%d]", i);
            GLint loc = glGetUniformLocation(debugCullProgram, name);
            if (loc >= 0)
            {
                glUniform4f(loc,
                            frustum.planes[i].normal.x,
                            frustum.planes[i].normal.y,
                            frustum.planes[i].normal.z,
                            frustum.planes[i].distance);
            }
        }

        GLint locSdfTex = glGetUniformLocation(debugCullProgram, "sdfTexture");
        GLint locOrigin = glGetUniformLocation(debugCullProgram, "gridOrigin");
        GLint locVoxel = glGetUniformLocation(debugCullProgram, "voxelSize");
        GLint locGrid = glGetUniformLocation(debugCullProgram, "gridSize");
        GLint locStep = glGetUniformLocation(debugCullProgram, "step");
        GLint locThreshold = glGetUniformLocation(debugCullProgram, "shellThreshold");
        GLint locRadius = glGetUniformLocation(debugCullProgram, "radius");
        GLint locRadiusMul = glGetUniformLocation(debugCullProgram, "radiusMultiplier");
        GLint locTotal = glGetUniformLocation(debugCullProgram, "candidateCount");

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, sdfTexture3D);
        glBindSampler(0, sdfSampler);

        if (locSdfTex >= 0)
            glUniform1i(locSdfTex, 0);
        if (locOrigin >= 0)
            glUniform3f(locOrigin, gridOrigin.x, gridOrigin.y, gridOrigin.z);
        if (locVoxel >= 0)
            glUniform1f(locVoxel, voxelSize);
        if (locGrid >= 0)
            glUniform3i(locGrid, gridResolution, gridResolution, gridResolution);
        if (locStep >= 0)
            glUniform1ui(locStep, static_cast<unsigned int>(step));
        if (locThreshold >= 0)
            glUniform1f(locThreshold, shellThreshold);
        if (locRadius >= 0)
            glUniform1f(locRadius, voxelCullRadius);
        if (locRadiusMul >= 0)
            glUniform1f(locRadiusMul, cullRadiusMultiplier);
        if (locTotal >= 0)
            glUniform1ui(locTotal, totalCandidates);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboDebugVisible);

        unsigned int groups = (totalCandidates + 255u) / 256u;
        glDispatchCompute(groups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        unsigned int visibleCount = 0;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboDebugVisible);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned int), &visibleCount);

        if (visibleCount > debugMaxVisible)
            visibleCount = debugMaxVisible;

        if (visibleCount > 0)
        {
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 16,
                               static_cast<GLsizeiptr>(visibleCount * sizeof(DebugVisibleVoxel)),
                               debugVisibleCPU.data());
            DrawDebugVoxelsInstanced(debugVisibleCPU.data(),
                                     static_cast<int>(visibleCount),
                                     shellThreshold,
                                     cubeSize);
        }

        glBindSampler(0, 0);
        glBindTexture(GL_TEXTURE_3D, 0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        glUseProgram(0);
        return;
    }

    // CPU fallback path (frustum behavior mirrors grass culling)
    const float cpuCullRadius = voxelCullRadius * cullRadiusMultiplier;
    debugVisibleCPU.clear();
    for (int iz = 0; iz < gridResolution; iz += step)
    {
        for (int iy = 0; iy < gridResolution; iy += step)
        {
            for (int ix = 0; ix < gridResolution; ix += step)
            {
                size_t index = static_cast<size_t>(ix) + static_cast<size_t>(iy) * static_cast<size_t>(gridResolution) + static_cast<size_t>(iz) * static_cast<size_t>(gridResolution) * static_cast<size_t>(gridResolution);

                float dist = sdfCPU[index];
                float absDist = fabsf(dist);
                if (absDist > shellThreshold)
                    continue;

                Vector3 worldPos = {
                    gridOrigin.x + (static_cast<float>(ix) + 0.5f) * voxelSize,
                    gridOrigin.y + (static_cast<float>(iy) + 0.5f) * voxelSize,
                    gridOrigin.z + (static_cast<float>(iz) + 0.5f) * voxelSize};

                if (!IsPointInFrustum(frustum, worldPos, cpuCullRadius))
                    continue;

                debugVisibleCPU.push_back({worldPos.x, worldPos.y, worldPos.z, dist});
            }
        }
    }

    DrawDebugVoxelsInstanced(debugVisibleCPU.data(),
                             static_cast<int>(debugVisibleCPU.size()),
                             shellThreshold,
                             cubeSize);
}
// ===========================================================================
// Collision queries – batch (GPU path)
// ===========================================================================

void SDFCollisionSystem::QueryCollisionBatch(const Vector3 *positions, const float *radii,
                                             SDFCollisionResult *results, int count)
{
    if (!initialized || !sdfReady || !enabled || count <= 0)
    {
        for (int i = 0; i < count; i++)
        {
            results[i].pushVector = {0, 0, 0};
            results[i].surfaceNormal = {0, 1, 0};
            results[i].penetrationDepth = 0.0f;
            results[i].colliding = false;
        }
        return;
    }

    // ----- Upload entity data -----
    std::vector<GPUCollisionEntity> entityData(static_cast<size_t>(count));
    for (int i = 0; i < count; i++)
    {
        entityData[i].positionRadius[0] = positions[i].x;
        entityData[i].positionRadius[1] = positions[i].y;
        entityData[i].positionRadius[2] = positions[i].z;
        entityData[i].positionRadius[3] = radii[i];
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboEntities);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 static_cast<GLsizeiptr>(count * sizeof(GPUCollisionEntity)),
                 entityData.data(), GL_STREAM_DRAW);

    // ----- Allocate result buffer -----
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboResults);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 static_cast<GLsizeiptr>(count * sizeof(GPUCollisionResultData)),
                 nullptr, GL_STREAM_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // ----- Dispatch collision compute shader -----
    glUseProgram(collisionProgram);

    // Bind SSBOs
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboEntities);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboResults);

    // Bind 3D SDF texture as a sampler on texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, sdfTexture3D);
    glBindSampler(0, sdfSampler);

    GLint locSdfTex = glGetUniformLocation(collisionProgram, "sdfTexture");
    GLint locGridOriginC = glGetUniformLocation(collisionProgram, "gridOrigin");
    GLint locVoxelSizeC = glGetUniformLocation(collisionProgram, "voxelSize");
    GLint locGridSizeC = glGetUniformLocation(collisionProgram, "gridSize");
    GLint locEntityCount = glGetUniformLocation(collisionProgram, "entityCount");

    glUniform1i(locSdfTex, 0);
    glUniform3f(locGridOriginC, gridOrigin.x, gridOrigin.y, gridOrigin.z);
    glUniform1f(locVoxelSizeC, voxelSize);
    glUniform3i(locGridSizeC, gridResolution, gridResolution, gridResolution);
    glUniform1ui(locEntityCount, static_cast<unsigned int>(count));

    unsigned int numGroups = (static_cast<unsigned int>(count) + 63) / 64;
    glDispatchCompute(numGroups, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // ----- Read back results -----
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboResults);
    std::vector<GPUCollisionResultData> gpuResults(static_cast<size_t>(count));
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                       static_cast<GLsizeiptr>(count * sizeof(GPUCollisionResultData)),
                       gpuResults.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Unbind sampler and texture
    glBindSampler(0, 0);
    glBindTexture(GL_TEXTURE_3D, 0);
    glUseProgram(0);

    // ----- Convert GPU results to SDFCollisionResult structs -----
    for (int i = 0; i < count; i++)
    {
        const auto &gr = gpuResults[i];
        results[i].pushVector = {gr.pushVector[0], gr.pushVector[1], gr.pushVector[2]};
        results[i].penetrationDepth = gr.pushVector[3];
        results[i].surfaceNormal = {gr.surfaceNormal[0], gr.surfaceNormal[1], gr.surfaceNormal[2]};
        results[i].colliding = (gr.surfaceNormal[3] > 0.5f);
    }
}

// ===========================================================================
// Iterative resolution
// ===========================================================================

Vector3 SDFCollisionSystem::ResolvePosition(Vector3 position, float radius, int maxIterations) const
{
    if (!sdfReady || !enabled)
        return position;

    for (int iter = 0; iter < maxIterations; iter++)
    {
        SDFCollisionResult result = QueryCollision(position, radius);
        if (!result.colliding)
            break;

        // Push out along the surface normal by the penetration amount
        position = Vector3Add(position, result.pushVector);
    }

    return position;
}

// ===========================================================================
// Debug visualisation
// ===========================================================================

void SDFCollisionSystem::DrawDebugSlice(Camera3D camera, float yLevel) const
{
    if (!sdfReady)
        return;

    // Determine the Y voxel index closest to yLevel
    float fy = (yLevel - gridOrigin.y) / voxelSize - 0.5f;
    int iy = ClampInt(static_cast<int>(roundf(fy)), 0, gridResolution - 1);

    float quadSize = voxelSize * 0.9f; // slightly smaller to see gaps

    int maxIdx = gridResolution - 1;
    int step = (gridResolution > 128) ? 2 : 1; // downsample for large grids

    for (int iz = 0; iz < gridResolution; iz += step)
    {
        for (int ix = 0; ix < gridResolution; ix += step)
        {
            size_t index = static_cast<size_t>(ix) + static_cast<size_t>(iy) * static_cast<size_t>(gridResolution) + static_cast<size_t>(iz) * static_cast<size_t>(gridResolution) * static_cast<size_t>(gridResolution);
            float dist = sdfCPU[index];

            // Map distance to color: blue = positive (safe), red = negative (inside)
            Color color;
            float absDist = fabsf(dist);
            float intensity = Clamp01(1.0f - absDist / (voxelSize * 10.0f));
            unsigned char a = static_cast<unsigned char>(intensity * 180.0f);

            if (dist < 0.0f)
                color = {255, 0, 0, a}; // red = inside
            else
                color = {0, 100, 255, a}; // blue = outside

            if (a < 10)
                continue; // skip nearly transparent quads

            float wx = gridOrigin.x + (static_cast<float>(ix) + 0.5f) * voxelSize;
            float wz = gridOrigin.z + (static_cast<float>(iz) + 0.5f) * voxelSize;

            DrawCube({wx, yLevel + 0.01f, wz}, quadSize * step, 0.02f, quadSize * step, color);
        }
    }
}

void SDFCollisionSystem::DrawDebugBounds(Color color) const
{
    if (!sdfReady)
        return;

    float extent = voxelSize * static_cast<float>(gridResolution);
    Vector3 center = {
        gridOrigin.x + extent * 0.5f,
        gridOrigin.y + extent * 0.5f,
        gridOrigin.z + extent * 0.5f};

    DrawCubeWires(center, extent, extent, extent, color);
}