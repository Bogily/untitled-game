#include "ForwardPlusSystem.h"
#include "glad/glad.h"
#include <cstdio>

ForwardPlusSystem gForwardPlus;

ForwardPlusSystem::ForwardPlusSystem()
    : pbrShader({0}),
      lightCullingShader({0}),
      initialized(false),
      lightCount(0),
      screenWidth(0),
      screenHeight(0),
      numTilesX(0),
      numTilesY(0),
      lightBuffer(0),
      visibleLightIndicesBuffer(0),
      lightGridBuffer(0)
{
}

ForwardPlusSystem::~ForwardPlusSystem()
{
    Shutdown();
}

void ForwardPlusSystem::Init(int width, int height)
{
    if (initialized)
    {
        TraceLog(LOG_WARNING, "ForwardPlusSystem: Already initialized");
        return;
    }

    screenWidth = width;
    screenHeight = height;

    // Calculate number of tiles
    numTilesX = (screenWidth + FORWARD_PLUS_TILE_SIZE - 1) / FORWARD_PLUS_TILE_SIZE;
    numTilesY = (screenHeight + FORWARD_PLUS_TILE_SIZE - 1) / FORWARD_PLUS_TILE_SIZE;

    TraceLog(LOG_INFO, "ForwardPlusSystem: Screen %dx%d, Tiles %dx%d", width, height, numTilesX, numTilesY);

    // Load Forward+ PBR shader
    pbrShader = LoadShader("assets/shader/pbr_forward_plus.vs", "assets/shader/pbr_forward_plus.fs");
    if (pbrShader.id == 0)
    {
        TraceLog(LOG_ERROR, "ForwardPlusSystem: Failed to load PBR shader!");
        initialized = false;
        return;
    }

    // Load light culling compute shader
    lightCullingShader = LoadShader(nullptr, "assets/shader/light_culling.comp");
    if (lightCullingShader.id == 0)
    {
        TraceLog(LOG_ERROR, "ForwardPlusSystem: Failed to load light culling compute shader!");
        UnloadShader(pbrShader);
        pbrShader.id = 0;
        initialized = false;
        return;
    }

    TraceLog(LOG_INFO, "ForwardPlusSystem: Shaders loaded (PBR: %d, Culling: %d)", pbrShader.id, lightCullingShader.id);

    // Create GPU buffers
    CreateComputeBuffers();

    // Reserve space for lights
    lights.reserve(FORWARD_PLUS_MAX_LIGHTS);

    // Set screen size uniform
    float screenSize[2] = {(float)screenWidth, (float)screenHeight};
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "screenSize"), screenSize, SHADER_UNIFORM_VEC2);

    initialized = true;
    TraceLog(LOG_INFO, "ForwardPlusSystem: Initialized successfully");
}

void ForwardPlusSystem::Shutdown()
{
    if (pbrShader.id > 0)
    {
        UnloadShader(pbrShader);
        pbrShader.id = 0;
    }

    if (lightCullingShader.id > 0)
    {
        UnloadShader(lightCullingShader);
        lightCullingShader.id = 0;
    }

    DestroyComputeBuffers();

    lights.clear();
    lightCount = 0;
    initialized = false;

    TraceLog(LOG_INFO, "ForwardPlusSystem: Shutdown complete");
}

void ForwardPlusSystem::CreateComputeBuffers()
{
    int totalTiles = numTilesX * numTilesY;

    // Create light buffer (SSBO)
    glGenBuffers(1, &lightBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ForwardPlusLight) * FORWARD_PLUS_MAX_LIGHTS, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuffer);

    // Create visible light indices buffer (SSBO)
    glGenBuffers(1, &visibleLightIndicesBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleLightIndicesBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned int) * totalTiles * FORWARD_PLUS_MAX_LIGHTS_PER_TILE, nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, visibleLightIndicesBuffer);

    // Create light grid buffer (SSBO) - stores offset and count per tile
    glGenBuffers(1, &lightGridBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightGridBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned int) * 2 * totalTiles, nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, lightGridBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    TraceLog(LOG_INFO, "ForwardPlusSystem: Created compute buffers (Tiles: %d)", totalTiles);
}

void ForwardPlusSystem::DestroyComputeBuffers()
{
    if (lightBuffer)
    {
        glDeleteBuffers(1, &lightBuffer);
        lightBuffer = 0;
    }

    if (visibleLightIndicesBuffer)
    {
        glDeleteBuffers(1, &visibleLightIndicesBuffer);
        visibleLightIndicesBuffer = 0;
    }

    if (lightGridBuffer)
    {
        glDeleteBuffers(1, &lightGridBuffer);
        lightGridBuffer = 0;
    }
}

void ForwardPlusSystem::ApplyToModel(Model &model, const Vector4 &albedo, float metallic, float roughness)
{
    if (!initialized)
    {
        TraceLog(LOG_WARNING, "ForwardPlusSystem: Not initialized, cannot apply to model");
        return;
    }

    if (model.materialCount <= 0)
        return;

    // Assign Forward+ PBR shader to the model's first material
    model.materials[0].shader = pbrShader;
    model.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    model.materials[0].maps[MATERIAL_MAP_METALNESS].value = metallic;
    model.materials[0].maps[MATERIAL_MAP_ROUGHNESS].value = roughness;

    // Set material uniforms
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "albedoColor"), &albedo, SHADER_UNIFORM_VEC4);
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "metallicValue"), &metallic, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "roughnessValue"), &roughness, SHADER_UNIFORM_FLOAT);

    TraceLog(LOG_INFO, "ForwardPlusSystem: Applied Forward+ PBR shader to model");
}

void ForwardPlusSystem::Update(const Camera &camera)
{
    if (!initialized)
        return;

    // Upload light data to GPU
    UploadLightData();

    // Perform light culling
    PerformLightCulling(camera);

    // Update shader uniforms
    UpdateShaderUniforms(camera);
}

void ForwardPlusSystem::UploadLightData()
{
    if (lightCount == 0)
        return;

    // Upload light data to SSBO
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ForwardPlusLight) * lightCount, lights.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ForwardPlusSystem::PerformLightCulling(const Camera &camera)
{
    if (lightCount == 0)
        return;

    // Get matrices
    Matrix viewMatrix = GetCameraMatrix(camera);
    Matrix projMatrix = MatrixPerspective(camera.fovy * DEG2RAD, (float)screenWidth / (float)screenHeight, 0.1f, 1000.0f);

    // Set uniforms for compute shader
    SetShaderValueMatrix(lightCullingShader, GetShaderLocation(lightCullingShader, "viewMatrix"), viewMatrix);
    SetShaderValueMatrix(lightCullingShader, GetShaderLocation(lightCullingShader, "projectionMatrix"), projMatrix);

    float screenSize[2] = {(float)screenWidth, (float)screenHeight};
    SetShaderValue(lightCullingShader, GetShaderLocation(lightCullingShader, "screenSize"), screenSize, SHADER_UNIFORM_VEC2);
    SetShaderValue(lightCullingShader, GetShaderLocation(lightCullingShader, "numLights"), &lightCount, SHADER_UNIFORM_INT);

    // Bind SSBOs
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, visibleLightIndicesBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, lightGridBuffer);

    // Dispatch compute shader
    glUseProgram(lightCullingShader.id);
    glDispatchCompute(numTilesX, numTilesY, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(0);
}

void ForwardPlusSystem::UpdateShaderUniforms(const Camera &camera)
{
    // Update camera position
    float camPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "viewPos"), camPos, SHADER_UNIFORM_VEC3);

    // Set number of lights
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "numLights"), &lightCount, SHADER_UNIFORM_INT);

    // Bind SSBOs for fragment shader to read
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, visibleLightIndicesBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, lightGridBuffer);
}

void ForwardPlusSystem::CreatePointLight(const Vector3 &pos, const Vector4 &color, float intensity, float radius)
{
    if (lightCount >= FORWARD_PLUS_MAX_LIGHTS)
    {
        TraceLog(LOG_WARNING, "ForwardPlusSystem: Cannot create more lights, max %d reached", FORWARD_PLUS_MAX_LIGHTS);
        return;
    }

    ForwardPlusLight light;
    light.type = 1; // point light
    light.enabled = 1;
    light.position = pos;
    light.radius = radius;
    light.color = color;
    light.intensity = intensity;

    lights.push_back(light);
    lightCount++;

    TraceLog(LOG_INFO, "ForwardPlusSystem: Created point light %d at (%.2f, %.2f, %.2f) radius %.2f intensity %.2f",
             lightCount - 1, pos.x, pos.y, pos.z, radius, intensity);
}

void ForwardPlusSystem::CreateDirectionalLight(const Vector3 &direction, const Vector4 &color, float intensity)
{
    if (lightCount >= FORWARD_PLUS_MAX_LIGHTS)
    {
        TraceLog(LOG_WARNING, "ForwardPlusSystem: Cannot create more lights, max %d reached", FORWARD_PLUS_MAX_LIGHTS);
        return;
    }

    ForwardPlusLight light;
    light.type = 2; // directional light
    light.enabled = 1;
    light.position = Vector3Normalize(direction); // Store normalized direction
    light.radius = 0.0f;                          // Not used for directional lights
    light.color = color;
    light.intensity = intensity;

    lights.push_back(light);
    lightCount++;

    TraceLog(LOG_INFO, "ForwardPlusSystem: Created directional light %d in direction (%.2f, %.2f, %.2f) intensity %.2f",
             lightCount - 1, direction.x, direction.y, direction.z, intensity);
}

void ForwardPlusSystem::UpdateLight(int index, const Vector3 &pos, const Vector4 &color, float intensity)
{
    if (index < 0 || index >= lightCount)
        return;

    lights[index].position = pos;
    lights[index].color = color;
    lights[index].intensity = intensity;
}

void ForwardPlusSystem::ClearLights()
{
    lights.clear();
    lightCount = 0;
    TraceLog(LOG_INFO, "ForwardPlusSystem: Cleared all lights");
}

void ForwardPlusSystem::DrawDebugLights()
{
    if (!initialized)
        return;

    for (int i = 0; i < lightCount; i++)
    {
        if (lights[i].enabled == 0)
            continue;

        if (lights[i].type == 1) // Point light
        {
            Color lightColor = {
                (unsigned char)(lights[i].color.x * 255),
                (unsigned char)(lights[i].color.y * 255),
                (unsigned char)(lights[i].color.z * 255),
                255};
            DrawSphere(lights[i].position, 0.2f, lightColor);
            DrawSphereWires(lights[i].position, lights[i].radius, 8, 8, ColorAlpha(lightColor, 0.3f));
        }
    }
}
