#include "LightRenderer.h"
#include "rlgl.h"
#include <algorithm>
#include <cstdio>

LightRenderer::LightRenderer()
    : pbrShader({0}),
      initialized(false),
      lightCount(0),
      ambientLight({0.05f, 0.05f, 0.05f})
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

void LightRenderer::Update(const Camera &camera, int maxActiveLights)
{
    if (!initialized)
        return;

    // Cull and sort lights by distance to camera
    CullAndSortLights(camera.position, maxActiveLights);

    // Update camera position uniform
    float camPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "viewPos"), camPos, SHADER_UNIFORM_VEC3);

    // Upload light data
    UploadLightData();
}

void LightRenderer::UploadLightData()
{
    // Upload ambient light uniform
    float ambient[3] = {ambientLight.x, ambientLight.y, ambientLight.z};
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "ambientLight"), ambient, SHADER_UNIFORM_VEC3);

    // Count enabled lights
    int enabledCount = 0;
    for (int i = 0; i < lightCount; i++)
    {
        if (lights[i].enabled == 1)
            enabledCount++;
    }

    // Upload active light count
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "numActiveLights"), &enabledCount, SHADER_UNIFORM_INT);

    // Upload individual light data
    for (int i = 0; i < lightCount && i < LIGHT_MAX_LIGHTS; i++)
    {
        char uniformName[64];

        sprintf(uniformName, "lights_type[%d]", i);
        SetShaderValue(pbrShader, GetShaderLocation(pbrShader, uniformName), &lights[i].type, SHADER_UNIFORM_INT);

        sprintf(uniformName, "lights_enabled[%d]", i);
        SetShaderValue(pbrShader, GetShaderLocation(pbrShader, uniformName), &lights[i].enabled, SHADER_UNIFORM_INT);

        sprintf(uniformName, "lights_positionRadius[%d]", i);
        SetShaderValue(pbrShader, GetShaderLocation(pbrShader, uniformName), &lights[i].positionRadius, SHADER_UNIFORM_VEC4);

        sprintf(uniformName, "lights_color[%d]", i);
        SetShaderValue(pbrShader, GetShaderLocation(pbrShader, uniformName), &lights[i].color, SHADER_UNIFORM_VEC4);

        sprintf(uniformName, "lights_intensity[%d]", i);
        SetShaderValue(pbrShader, GetShaderLocation(pbrShader, uniformName), &lights[i].intensity, SHADER_UNIFORM_FLOAT);
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

void LightRenderer::SetAmbientLight(const Vector3 &ambient)
{
    ambientLight = ambient;
    TraceLog(LOG_INFO, "LightRenderer: Ambient light set to (%.2f, %.2f, %.2f)", ambient.x, ambient.y, ambient.z);
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

void LightRenderer::CullAndSortLights(const Vector3 &cameraPos, int maxActiveLights)
{
    if (lightCount == 0)
        return;

    // Sort lights by distance to camera (closest first)
    std::sort(lights.begin(), lights.begin() + lightCount, [&](const Light &a, const Light &b)
              {
        // Directional lights always stay at the front
        if (a.type == 2 && b.type != 2) return true;
        if (b.type == 2 && a.type != 2) return false;
        
        // Both directional or both point lights - sort by distance
        Vector3 posA = {a.positionRadius.x, a.positionRadius.y, a.positionRadius.z};
        Vector3 posB = {b.positionRadius.x, b.positionRadius.y, b.positionRadius.z};
        float distA = Vector3Distance(cameraPos, posA);
        float distB = Vector3Distance(cameraPos, posB);
        return distA < distB; });

    // Enable the closest lights up to maxActiveLights
    int lightsToEnable = (lightCount < maxActiveLights) ? lightCount : maxActiveLights;
    for (int i = 0; i < lightsToEnable; i++)
    {
        lights[i].enabled = 1;
    }

    // Disable lights beyond the limit
    for (int i = lightsToEnable; i < lightCount; i++)
    {
        lights[i].enabled = 0;
    }
}
