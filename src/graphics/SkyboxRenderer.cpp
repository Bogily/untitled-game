#include "SkyboxRenderer.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

SkyboxRenderer::SkyboxRenderer() : time(0.0f),
                                   skyColor({0.1f, 0.2f, 0.9f}),
                                   cloudColor({1.0f, 1.0f, 1.0f}),
                                   sunDirection({0.3f, 0.5f, 0.8f}),
                                   sunColor({1.0f, 0.3f, 0.3f}),
                                   cloudDensity(0.6f),
                                   cloudHeight(200.0f),
                                   cloudScale(0.4f),
                                   cloudSpeed(0.3f),
                                   cloudCoverage(0.5f),
                                   cloudOffset({0.0f, 0.0f, 0.0f}),
                                   loaded(false)
{
    shader = {0};
    cube = {0};
}

SkyboxRenderer::~SkyboxRenderer()
{
}

void SkyboxRenderer::Load(const char *vsPath, const char *fsPath)
{
    if (loaded)
    {
        Unload();
    }

    // Load skybox shader
    shader = LoadShader(vsPath, fsPath);

    // Get shader uniform locations
    timeLoc = GetShaderLocation(shader, "time");
    skyColorLoc = GetShaderLocation(shader, "skyColor");
    cloudColorLoc = GetShaderLocation(shader, "cloudColor");
    sunDirectionLoc = GetShaderLocation(shader, "sunDirection");
    sunColorLoc = GetShaderLocation(shader, "sunColor");

    // Get 3D cloud uniform locations
    cloudDensityLoc = GetShaderLocation(shader, "cloudDensity");
    cloudHeightLoc = GetShaderLocation(shader, "cloudHeight");
    cloudScaleLoc = GetShaderLocation(shader, "cloudScale");
    cloudSpeedLoc = GetShaderLocation(shader, "cloudSpeed");
    cloudCoverageLoc = GetShaderLocation(shader, "cloudCoverage");
    cloudOffsetLoc = GetShaderLocation(shader, "cloudOffset");

    // Generate a cube mesh for the skybox
    cube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    cube.materials[0].shader = shader;
    loaded = true;

    TraceLog(LOG_INFO, "SkyboxRenderer: Loaded successfully");
}

void SkyboxRenderer::SetSkyColor(Vector3 color)
{
    skyColor = color;
}

void SkyboxRenderer::SetCloudColor(Vector3 color)
{
    cloudColor = color;
}

void SkyboxRenderer::SetSunDirection(Vector3 direction)
{
    sunDirection = Vector3Normalize(direction);
}

void SkyboxRenderer::SetSunColor(Vector3 color)
{
    sunColor = color;
}

void SkyboxRenderer::SetCloudDensity(float density)
{
    cloudDensity = density;
}

void SkyboxRenderer::SetCloudHeight(float height)
{
    cloudHeight = height;
}

void SkyboxRenderer::SetCloudScale(float scale)
{
    cloudScale = scale;
}

void SkyboxRenderer::SetCloudSpeed(float speed)
{
    cloudSpeed = speed;
}

void SkyboxRenderer::SetCloudCoverage(float coverage)
{
    cloudCoverage = coverage;
}

void SkyboxRenderer::Update(float deltaTime)
{
    time += deltaTime;

    // Animate clouds by updating offset
    cloudOffset.x += deltaTime * cloudSpeed * 2.0f;
    cloudOffset.z += deltaTime * cloudSpeed * 1.5f;
}

void SkyboxRenderer::Draw(Camera3D camera)
{
    if (!loaded)
        return;

    // Update shader uniforms
    SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, skyColorLoc, &skyColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, cloudColorLoc, &cloudColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, sunDirectionLoc, &sunDirection, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, sunColorLoc, &sunColor, SHADER_UNIFORM_VEC3);

    // Update 3D cloud uniforms
    SetShaderValue(shader, cloudDensityLoc, &cloudDensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, cloudHeightLoc, &cloudHeight, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, cloudScaleLoc, &cloudScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, cloudSpeedLoc, &cloudSpeed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, cloudCoverageLoc, &cloudCoverage, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, cloudOffsetLoc, &cloudOffset, SHADER_UNIFORM_VEC3);

    // Disable depth writing for skybox (it should always be behind everything)
    rlDisableDepthMask();

    // Disable backface culling so we can see the inside of the cube
    rlDisableBackfaceCulling();

    // Push matrix to modify the model matrix
    rlPushMatrix();

    // Scale the cube to be very large
    rlScalef(1000.0f, 1000.0f, 1000.0f);

    // Center the skybox on the camera position
    rlTranslatef(camera.position.x / 1000.0f, camera.position.y / 1000.0f, camera.position.z / 1000.0f);

    // Draw the skybox cube
    DrawModel(cube, Vector3Zero(), 1.0f, WHITE);

    rlPopMatrix();

    // Re-enable depth writing and backface culling
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}

void SkyboxRenderer::Unload()
{
    if (!loaded)
        return;

    if (cube.meshCount > 0)
    {
        UnloadModel(cube);
        cube = {0};
    }

    if (shader.id > 0)
    {
        UnloadShader(shader);
        shader = {0};
    }

    loaded = false;
    TraceLog(LOG_INFO, "SkyboxRenderer: Unloaded");
}
