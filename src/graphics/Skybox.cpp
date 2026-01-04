#include "Skybox.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

Skybox::Skybox() : time(0.0f),
                   skyColor({0.1f, 0.2f, 0.9f}),
                   cloudColor({1.0f, 1.0f, 1.0f}),
                   sunDirection({0.3f, 0.5f, 0.8f}),
                   sunColor({1.0f, 0.3f, 0.3f}),
                   cloudDensity(0.6f),
                   cloudHeight(200.0f),
                   cloudScale(0.4f),
                   cloudSpeed(0.3f),
                   cloudCoverage(0.5f),
                   cloudOffset({0.0f, 0.0f, 0.0f})
{
    // Constructor - initialization happens in Load()
}

Skybox::~Skybox()
{
    // Destructor
}

void Skybox::Load(const char *vsPath, const char *fsPath)
{
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

    TraceLog(LOG_INFO, "Skybox loaded successfully");
}

void Skybox::SetSkyColor(Vector3 color)
{
    skyColor = color;
}

void Skybox::SetCloudColor(Vector3 color)
{
    cloudColor = color;
}

void Skybox::SetSunDirection(Vector3 direction)
{
    sunDirection = Vector3Normalize(direction);
}

void Skybox::SetSunColor(Vector3 color)
{
    sunColor = color;
}

void Skybox::SetCloudDensity(float density)
{
    cloudDensity = density;
}

void Skybox::SetCloudHeight(float height)
{
    cloudHeight = height;
}

void Skybox::SetCloudScale(float scale)
{
    cloudScale = scale;
}

void Skybox::SetCloudSpeed(float speed)
{
    cloudSpeed = speed;
}

void Skybox::SetCloudCoverage(float coverage)
{
    cloudCoverage = coverage;
}

void Skybox::Update(float deltaTime)
{
    time += deltaTime;

    // Animate clouds by updating offset
    cloudOffset.x += deltaTime * cloudSpeed * 2.0f;
    cloudOffset.z += deltaTime * cloudSpeed * 1.5f;
}

void Skybox::Draw(Camera3D camera)
{
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

void Skybox::Unload()
{
    UnloadShader(shader);
    UnloadModel(cube);
    TraceLog(LOG_INFO, "Skybox unloaded");
}
