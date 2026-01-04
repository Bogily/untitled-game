#include "WaterRenderer.h"
#include <rlgl.h>
#include <glad/glad.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

WaterRenderer::WaterRenderer()
    : waterLevel(-0.5f), waterWidth(50.0f), waterDepth(50.0f), elapsedTime(0.0f), lightDirection({0.3f, 0.5f, 0.8f}), computeProgram(0), ssboHeightField(0), ssboVelocityField(0), ssboPrevHeightField(0), gridWidth(128), gridHeight(128), useComputeShader(true), damping(0.995f), waveSpeed(2.5f), waveStrength(1.0f), windDirection({1.0f, 0.5f}), windStrength(0.3f), initialized(false)
{
}

WaterRenderer::~WaterRenderer()
{
    Cleanup();
}

void WaterRenderer::Init()
{
    if (initialized)
    {
        std::cout << "WaterRenderer already initialized!" << std::endl;
        return;
    }

    std::cout << "Initializing WaterRenderer..." << std::endl;

    // Load water shader
    waterShader = LoadShader("assets/shader/water.vs", "assets/shader/water.fs");

    if (waterShader.id == 0)
    {
        std::cerr << "Failed to load water shader!" << std::endl;
        return;
    }

    // Get shader uniform locations
    timeLoc = GetShaderLocation(waterShader, "time");
    viewPosLoc = GetShaderLocation(waterShader, "viewPos");
    lightDirLoc = GetShaderLocation(waterShader, "lightDir");
    mvpLoc = GetShaderLocation(waterShader, "mvp");
    matModelLoc = GetShaderLocation(waterShader, "matModel");
    matNormalLoc = GetShaderLocation(waterShader, "matNormal");

    // Match grid resolution for compute shader
    int subdivisions = useComputeShader ? gridWidth : 64;

    // Create water plane mesh with subdivision matching compute grid
    waterMesh = GenMeshPlane(waterWidth, waterDepth, subdivisions, subdivisions);

    // Create model from mesh
    waterModel = LoadModelFromMesh(waterMesh);

    // Apply water shader to the model's material
    waterModel.materials[0].shader = waterShader;

    // Set water color (base color, shader will override)
    waterModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){0, 100, 150, 217};

    // Initialize compute shader if enabled
    if (useComputeShader)
    {
        InitComputeShader();
    }

    initialized = true;
    std::cout << "WaterRenderer initialized successfully (Compute: " << (useComputeShader ? "ON" : "OFF") << ")" << std::endl;
}

void WaterRenderer::Update(float deltaTime, Camera3D camera)
{
    if (!initialized)
        return;

    elapsedTime += deltaTime;

    // Run compute shader simulation if enabled
    if (useComputeShader && computeProgram != 0)
    {
        RunComputeShader(deltaTime);
        UpdateMeshFromCompute();
    }

    // Update shader uniforms
    SetShaderValue(waterShader, timeLoc, &elapsedTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, viewPosLoc, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(waterShader, lightDirLoc, &lightDirection, SHADER_UNIFORM_VEC3);
}

void WaterRenderer::Draw()
{
    if (!initialized)
        return;

    // Enable blending for water transparency
    rlSetBlendMode(BLEND_ALPHA);

    // Draw water at specified level
    Vector3 waterPosition = {0.0f, waterLevel, 0.0f};
    DrawModel(waterModel, waterPosition, 1.0f, WHITE);

    // Reset blend mode
    rlSetBlendMode(BLEND_ALPHA);
}

void WaterRenderer::Cleanup()
{
    if (!initialized)
        return;

    // Clean up compute shader resources
    if (ssboHeightField != 0)
        glDeleteBuffers(1, &ssboHeightField);
    if (ssboVelocityField != 0)
        glDeleteBuffers(1, &ssboVelocityField);
    if (ssboPrevHeightField != 0)
        glDeleteBuffers(1, &ssboPrevHeightField);
    if (computeProgram != 0)
        glDeleteProgram(computeProgram);

    UnloadModel(waterModel);
    UnloadShader(waterShader);

    initialized = false;
    std::cout << "WaterRenderer cleaned up" << std::endl;
}

void WaterRenderer::SetLightDirection(Vector3 direction)
{
    lightDirection = Vector3Normalize(direction);
}

void WaterRenderer::SetWaterLevel(float yPosition)
{
    waterLevel = yPosition;
}

void WaterRenderer::SetWaterSize(float width, float depth)
{
    if (initialized)
    {
        std::cerr << "Cannot change water size after initialization!" << std::endl;
        return;
    }

    waterWidth = width;
    waterDepth = depth;
}

void WaterRenderer::SetUseComputeShader(bool enable)
{
    if (initialized)
    {
        std::cerr << "Cannot change compute shader setting after initialization!" << std::endl;
        return;
    }
    useComputeShader = enable;
}

void WaterRenderer::InitComputeShader()
{
    std::cout << "Initializing water compute shader..." << std::endl;

    // Load compute shader source
    std::ifstream file("assets/shader/water_simulation.comp");
    if (!file.is_open())
    {
        std::cerr << "Failed to open water compute shader file!" << std::endl;
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    const char *sourcePtr = source.c_str();

    // Compile compute shader
    unsigned int computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, &sourcePtr, nullptr);
    glCompileShader(computeShader);

    // Check compilation
    int success;
    char infoLog[512];
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(computeShader, 512, nullptr, infoLog);
        std::cerr << "Compute shader compilation failed:\n"
                  << infoLog << std::endl;
        glDeleteShader(computeShader);
        return;
    }

    // Create and link program
    computeProgram = glCreateProgram();
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);

    glGetProgramiv(computeProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(computeProgram, 512, nullptr, infoLog);
        std::cerr << "Compute shader program linking failed:\n"
                  << infoLog << std::endl;
        glDeleteShader(computeShader);
        glDeleteProgram(computeProgram);
        computeProgram = 0;
        return;
    }

    glDeleteShader(computeShader);

    // Get uniform locations
    compDeltaTimeLoc = glGetUniformLocation(computeProgram, "deltaTime");
    compTimeLoc = glGetUniformLocation(computeProgram, "time");
    compGridWidthLoc = glGetUniformLocation(computeProgram, "gridWidth");
    compGridHeightLoc = glGetUniformLocation(computeProgram, "gridHeight");
    compDampingLoc = glGetUniformLocation(computeProgram, "damping");
    compWaveSpeedLoc = glGetUniformLocation(computeProgram, "waveSpeed");
    compWaveStrengthLoc = glGetUniformLocation(computeProgram, "waveStrength");
    compWindDirectionLoc = glGetUniformLocation(computeProgram, "windDirection");
    compWindStrengthLoc = glGetUniformLocation(computeProgram, "windStrength");

    // Create SSBOs for height field, velocity, and previous height
    int gridSize = (gridWidth + 1) * (gridHeight + 1);
    std::vector<float> initialHeights(gridSize, 0.0f);
    std::vector<float> initialVelocities(gridSize, 0.0f);

    glGenBuffers(1, &ssboHeightField);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboHeightField);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gridSize * sizeof(float), initialHeights.data(), GL_DYNAMIC_COPY);

    glGenBuffers(1, &ssboVelocityField);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVelocityField);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gridSize * sizeof(float), initialVelocities.data(), GL_DYNAMIC_COPY);

    glGenBuffers(1, &ssboPrevHeightField);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboPrevHeightField);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gridSize * sizeof(float), initialHeights.data(), GL_DYNAMIC_COPY);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    std::cout << "Water compute shader initialized (Grid: " << gridWidth << "x" << gridHeight << ")" << std::endl;
}

void WaterRenderer::RunComputeShader(float deltaTime)
{
    if (computeProgram == 0)
        return;

    glUseProgram(computeProgram);

    // Set uniforms
    glUniform1f(compDeltaTimeLoc, deltaTime);
    glUniform1f(compTimeLoc, elapsedTime);
    glUniform1i(compGridWidthLoc, gridWidth + 1);
    glUniform1i(compGridHeightLoc, gridHeight + 1);
    glUniform1f(compDampingLoc, damping);
    glUniform1f(compWaveSpeedLoc, waveSpeed);
    glUniform1f(compWaveStrengthLoc, waveStrength);
    glUniform2f(compWindDirectionLoc, windDirection.x, windDirection.y);
    glUniform1f(compWindStrengthLoc, windStrength);

    // Bind SSBOs
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboHeightField);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboVelocityField);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboPrevHeightField);

    // Dispatch compute shader
    int groupsX = (gridWidth + 1 + 7) / 8;
    int groupsY = (gridHeight + 1 + 7) / 8;
    glDispatchCompute(groupsX, groupsY, 1);

    // Memory barrier
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(0);
}

void WaterRenderer::UpdateMeshFromCompute()
{
    if (ssboHeightField == 0)
        return;

    // Read back height data from GPU
    int gridSize = (gridWidth + 1) * (gridHeight + 1);
    std::vector<float> heights(gridSize);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboHeightField);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, gridSize * sizeof(float), heights.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Update mesh vertices with computed heights
    // The mesh is (gridWidth+1) x (gridHeight+1) vertices
    for (int i = 0; i < gridSize; i++)
    {
        waterMesh.vertices[i * 3 + 1] = heights[i]; // Update Y coordinate
    }

    // Recalculate normals for proper lighting
    // Simple finite difference for normals
    for (int y = 0; y <= gridHeight; y++)
    {
        for (int x = 0; x <= gridWidth; x++)
        {
            int idx = y * (gridWidth + 1) + x;

            // Get neighbor heights
            float hL = (x > 0) ? heights[y * (gridWidth + 1) + (x - 1)] : heights[idx];
            float hR = (x < gridWidth) ? heights[y * (gridWidth + 1) + (x + 1)] : heights[idx];
            float hD = (y > 0) ? heights[(y - 1) * (gridWidth + 1) + x] : heights[idx];
            float hU = (y < gridHeight) ? heights[(y + 1) * (gridWidth + 1) + x] : heights[idx];

            // Compute normal via cross product of tangent vectors
            float dx = hR - hL;
            float dz = hU - hD;

            Vector3 normal = Vector3Normalize((Vector3){-dx * 10.0f, 1.0f, -dz * 10.0f});

            waterMesh.normals[idx * 3 + 0] = normal.x;
            waterMesh.normals[idx * 3 + 1] = normal.y;
            waterMesh.normals[idx * 3 + 2] = normal.z;
        }
    }

    // Update mesh on GPU
    UpdateMeshBuffer(waterMesh, 0, waterMesh.vertices, waterMesh.vertexCount * 3 * sizeof(float), 0);
    UpdateMeshBuffer(waterMesh, 2, waterMesh.normals, waterMesh.vertexCount * 3 * sizeof(float), 0);
}
