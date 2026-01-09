#include "LightRenderer.h"
#include <cstdio>

LightRenderer::LightRenderer()
    : pbrShader({0}),
      initialized(false),
      lightCount(0)
{
}

LightRenderer::~LightRenderer()
{
    Shutdown();
}

void LightRenderer::Init(int width, int height)
{
    if (initialized)
    {
        TraceLog(LOG_WARNING, "LightRenderer: Already initialized");
        return;
    }

    // Load PBR shader
    pbrShader = LoadShader("assets/shader/pbr.vs", "assets/shader/pbr.fs");
    if (pbrShader.id == 0)
    {
        TraceLog(LOG_ERROR, "LightRenderer: Failed to load PBR shader!");
        return;
    }

    // Set number of lights uniform
    int numLights = LIGHT_MAX_LIGHTS;
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "numOfLights"), &numLights, SHADER_UNIFORM_INT);

    // Reserve space for lights
    lights.reserve(LIGHT_MAX_LIGHTS);

    initialized = true;
    TraceLog(LOG_INFO, "LightRenderer: PBR shader loaded (max %d lights)", LIGHT_MAX_LIGHTS);
}

void LightRenderer::Shutdown()
{
    if (!initialized)
        return;

    if (pbrShader.id > 0)
    {
        UnloadShader(pbrShader);
        pbrShader.id = 0;
    }

    lights.clear();
    lightCount = 0;
    initialized = false;

    TraceLog(LOG_INFO, "LightRenderer: Shutdown complete");
}

void LightRenderer::ApplyToModel(Model &model, const Vector4 &albedo, float metallic, float roughness)
{
    if (!initialized || model.materialCount <= 0)
        return;

    // Assign shader to the model's first material
    model.materials[0].shader = pbrShader;
    model.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    model.materials[0].maps[MATERIAL_MAP_METALNESS].value = metallic;
    model.materials[0].maps[MATERIAL_MAP_ROUGHNESS].value = roughness;

    // Set material uniforms
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "albedoColor"), &albedo, SHADER_UNIFORM_VEC4);
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "metallicValue"), &metallic, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "roughnessValue"), &roughness, SHADER_UNIFORM_FLOAT);
}

void LightRenderer::Update(const Camera &camera)
{
    if (!initialized)
        return;

    // Update camera position uniform
    float camPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "viewPos"), camPos, SHADER_UNIFORM_VEC3);

    // Upload light data
    UploadLightData();
}

void LightRenderer::UploadLightData()
{
    int maxLights = (lightCount < LIGHT_MAX_LIGHTS) ? lightCount : LIGHT_MAX_LIGHTS;

    for (int i = 0; i < maxLights; ++i)
    {
        char locName[64];

        // Upload position
        sprintf(locName, "lights[%d].position", i);
        int posLoc = GetShaderLocation(pbrShader, locName);
        float pos[3] = {lights[i].positionRadius.x, lights[i].positionRadius.y, lights[i].positionRadius.z};
        SetShaderValue(pbrShader, posLoc, pos, SHADER_UNIFORM_VEC3);

        // Upload color
        sprintf(locName, "lights[%d].color", i);
        int colLoc = GetShaderLocation(pbrShader, locName);
        SetShaderValue(pbrShader, colLoc, &lights[i].color, SHADER_UNIFORM_VEC4);

        // Upload intensity
        sprintf(locName, "lights[%d].intensity", i);
        int intLoc = GetShaderLocation(pbrShader, locName);
        SetShaderValue(pbrShader, intLoc, &lights[i].intensity, SHADER_UNIFORM_FLOAT);

        // Upload enabled
        sprintf(locName, "lights[%d].enabled", i);
        int enabledLoc = GetShaderLocation(pbrShader, locName);
        SetShaderValue(pbrShader, enabledLoc, &lights[i].enabled, SHADER_UNIFORM_INT);

        // Upload type
        sprintf(locName, "lights[%d].type", i);
        int typeLoc = GetShaderLocation(pbrShader, locName);
        SetShaderValue(pbrShader, typeLoc, &lights[i].type, SHADER_UNIFORM_INT);
    }
}

void LightRenderer::CreatePointLight(const Vector3 &pos, const Vector4 &color, float intensity, float radius)
{
    if (lightCount >= LIGHT_MAX_LIGHTS)
    {
        TraceLog(LOG_WARNING, "LightRenderer: Cannot create more lights, max %d reached", LIGHT_MAX_LIGHTS);
        return;
    }

    Light light;
    light.type = 1; // point light
    light.enabled = 1;
    light.positionRadius = {pos.x, pos.y, pos.z, radius};
    light.color = color;
    light.intensity = intensity;

    lights.push_back(light);
    lightCount++;

    TraceLog(LOG_INFO, "LightRenderer: Created point light %d at (%.2f, %.2f, %.2f)",
             lightCount - 1, pos.x, pos.y, pos.z);
}

void LightRenderer::CreateDirectionalLight(const Vector3 &direction, const Vector4 &color, float intensity)
{
    if (lightCount >= LIGHT_MAX_LIGHTS)
    {
        TraceLog(LOG_WARNING, "LightRenderer: Cannot create more lights, max %d reached", LIGHT_MAX_LIGHTS);
        return;
    }

    Light light;
    light.type = 2; // directional light
    light.enabled = 1;
    Vector3 dir = Vector3Normalize(direction);
    light.positionRadius = {dir.x, dir.y, dir.z, 0.0f}; // .w unused for directional lights
    light.color = color;
    light.intensity = intensity;

    lights.push_back(light);
    lightCount++;

    TraceLog(LOG_INFO, "LightRenderer: Created directional light %d in direction (%.2f, %.2f, %.2f)",
             lightCount - 1, direction.x, direction.y, direction.z);
}

void LightRenderer::UpdateLight(int index, const Vector3 &pos, const Vector4 &color, float intensity)
{
    if (index < 0 || index >= lightCount)
        return;

    lights[index].positionRadius.x = pos.x;
    lights[index].positionRadius.y = pos.y;
    lights[index].positionRadius.z = pos.z;
    lights[index].color = color;
    lights[index].intensity = intensity;
}

void LightRenderer::ClearLights()
{
    lights.clear();
    lightCount = 0;
    TraceLog(LOG_INFO, "LightRenderer: All lights cleared");
}

Vector3 LightRenderer::GetSunDirection() const
{
    for (int i = 0; i < lightCount; i++)
    {
        if (lights[i].type == 2 && lights[i].enabled == 1)
        {
            return {lights[i].positionRadius.x, lights[i].positionRadius.y, lights[i].positionRadius.z}; // Direction stored in positionRadius.xyz
        }
    }
    return {0.3f, 0.5f, 0.8f}; // Default sun direction
}

void LightRenderer::DrawDebugLights()
{
    for (int i = 0; i < lightCount; i++)
    {
        if (!lights[i].enabled)
            continue;

        if (lights[i].type == 1) // Point light
        {
            Color col = {
                (unsigned char)(lights[i].color.x * 255),
                (unsigned char)(lights[i].color.y * 255),
                (unsigned char)(lights[i].color.z * 255),
                255};
            Vector3 pos = {lights[i].positionRadius.x, lights[i].positionRadius.y, lights[i].positionRadius.z};
            DrawSphere(pos, 0.2f, col);
        }
    }
}
