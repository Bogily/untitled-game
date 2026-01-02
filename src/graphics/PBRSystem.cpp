#include "PBRSystem.h"
#include <cstdio>

PBRSystem gPBR;

PBRSystem::PBRSystem()
    : shader({0}), initialized(false), lightCount(0)
{
}

PBRSystem::~PBRSystem()
{
    Shutdown();
}

void PBRSystem::Init()
{
    if (initialized)
        return;

    // Load PBR shader
    shader = LoadShader("assets/shader/pbr.vs", "assets/shader/pbr.fs");
    if (shader.id == 0)
    {
        TraceLog(LOG_ERROR, "Failed to load PBR shader!");
        initialized = false;
        return;
    }

    TraceLog(LOG_INFO, "PBR shader loaded successfully (ID: %d)", shader.id);

    // Set number of lights uniform
    int numLights = PBR_MAX_LIGHTS;
    SetShaderValue(shader, GetShaderLocation(shader, "numOfLights"), &numLights, SHADER_UNIFORM_INT);

    // Create default world lights for realistic illumination
    CreatePointLight((Vector3){-5.0f, 4.0f, -5.0f}, (Vector4){1.0f, 0.9f, 0.8f, 1.0f}, 15.0f); // Warm white light
    CreatePointLight((Vector3){5.0f, 4.0f, 5.0f}, (Vector4){0.8f, 0.9f, 1.0f, 1.0f}, 15.0f);   // Cool white light
    CreatePointLight((Vector3){0.0f, 6.0f, 0.0f}, (Vector4){1.0f, 1.0f, 1.0f, 1.0f}, 20.0f);   // Center overhead light
    CreatePointLight((Vector3){8.0f, 3.0f, -7.0f}, (Vector4){0.9f, 0.8f, 0.6f, 1.0f}, 12.0f);  // Light near ramp

    initialized = true;
    TraceLog(LOG_INFO, "PBR system initialized with %d lights", lightCount);
}

void PBRSystem::Shutdown()
{
    if (shader.id > 0)
    {
        UnloadShader(shader);
        shader.id = 0;
    }
    lightCount = 0;
    initialized = false;
    TraceLog(LOG_INFO, "PBR system shutdown");
}

void PBRSystem::ApplyToModel(Model &model, const Vector4 &albedo, float metallic, float roughness)
{
    if (!initialized)
    {
        TraceLog(LOG_WARNING, "PBRSystem not initialized, cannot apply to model");
        return;
    }

    if (model.materialCount <= 0)
        return;

    // Assign PBR shader to the model's first material
    model.materials[0].shader = shader;
    model.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    model.materials[0].maps[MATERIAL_MAP_METALNESS].value = metallic;
    model.materials[0].maps[MATERIAL_MAP_ROUGHNESS].value = roughness;

    // Set material uniforms
    SetShaderValue(shader, GetShaderLocation(shader, "albedoColor"), &albedo, SHADER_UNIFORM_VEC4);
    SetShaderValue(shader, GetShaderLocation(shader, "metallicValue"), &metallic, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, GetShaderLocation(shader, "roughnessValue"), &roughness, SHADER_UNIFORM_FLOAT);

    TraceLog(LOG_INFO, "Applied PBR shader to model (albedo: %.2f,%.2f,%.2f, metallic: %.2f, roughness: %.2f)",
             albedo.x, albedo.y, albedo.z, metallic, roughness);
}

void PBRSystem::Update(const Camera &camera)
{
    if (!initialized)
        return;

    // Update camera position uniform
    float camPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(shader, GetShaderLocation(shader, "viewPos"), camPos, SHADER_UNIFORM_VEC3);

    // Update all lights
    for (int i = 0; i < lightCount; ++i)
    {
        UploadLightData(i);
    }
}

void PBRSystem::CreatePointLight(const Vector3 &pos, const Vector4 &color, float intensity)
{
    if (lightCount >= PBR_MAX_LIGHTS)
    {
        TraceLog(LOG_WARNING, "Cannot create more lights, max %d reached", PBR_MAX_LIGHTS);
        return;
    }

    lights[lightCount].type = 1; // point light
    lights[lightCount].enabled = 1;
    lights[lightCount].position = pos;
    lights[lightCount].color = color;
    lights[lightCount].intensity = intensity;

    UploadLightData(lightCount);
    lightCount++;

    TraceLog(LOG_INFO, "Created point light %d at (%.2f, %.2f, %.2f) with intensity %.2f",
             lightCount - 1, pos.x, pos.y, pos.z, intensity);
}

void PBRSystem::UpdateLight(int index, const Vector3 &pos, const Vector4 &color, float intensity)
{
    if (index < 0 || index >= lightCount)
        return;

    lights[index].position = pos;
    lights[index].color = color;
    lights[index].intensity = intensity;

    UploadLightData(index);
}

void PBRSystem::UploadLightData(int lightIndex)
{
    if (!initialized || lightIndex < 0 || lightIndex >= PBR_MAX_LIGHTS)
        return;

    char locName[64];

    // Upload position
    sprintf(locName, "lights[%d].position", lightIndex);
    int posLoc = GetShaderLocation(shader, locName);
    float pos[3] = {lights[lightIndex].position.x, lights[lightIndex].position.y, lights[lightIndex].position.z};
    SetShaderValue(shader, posLoc, pos, SHADER_UNIFORM_VEC3);

    // Upload color
    sprintf(locName, "lights[%d].color", lightIndex);
    int colLoc = GetShaderLocation(shader, locName);
    SetShaderValue(shader, colLoc, &lights[lightIndex].color, SHADER_UNIFORM_VEC4);

    // Upload intensity
    sprintf(locName, "lights[%d].intensity", lightIndex);
    int intLoc = GetShaderLocation(shader, locName);
    SetShaderValue(shader, intLoc, &lights[lightIndex].intensity, SHADER_UNIFORM_FLOAT);

    // Upload enabled
    sprintf(locName, "lights[%d].enabled", lightIndex);
    int enabledLoc = GetShaderLocation(shader, locName);
    SetShaderValue(shader, enabledLoc, &lights[lightIndex].enabled, SHADER_UNIFORM_INT);

    // Upload type
    sprintf(locName, "lights[%d].type", lightIndex);
    int typeLoc = GetShaderLocation(shader, locName);
    SetShaderValue(shader, typeLoc, &lights[lightIndex].type, SHADER_UNIFORM_INT);
}

void PBRSystem::DrawDebugLights()
{
    for (int i = 0; i < lightCount; ++i)
    {
        if (!lights[i].enabled)
            continue;

        // Convert color from float to Color
        Color c = {
            (unsigned char)(lights[i].color.x * 255),
            (unsigned char)(lights[i].color.y * 255),
            (unsigned char)(lights[i].color.z * 255),
            (unsigned char)(lights[i].color.w * 255)};

        DrawSphere(lights[i].position, 0.1f, c);
    }
}
