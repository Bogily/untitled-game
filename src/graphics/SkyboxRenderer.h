#pragma once
#include "raylib.h"

class SkyboxRenderer
{
private:
    Shader shader;
    Model cube;
    int timeLoc;
    int skyColorLoc;
    int cloudColorLoc;
    int sunDirectionLoc;
    int sunColorLoc;

    // 3D Cloud uniform locations
    int cloudDensityLoc;
    int cloudHeightLoc;
    int cloudScaleLoc;
    int cloudSpeedLoc;
    int cloudCoverageLoc;
    int cloudOffsetLoc;

    float time;
    Vector3 skyColor;
    Vector3 cloudColor;
    Vector3 sunDirection;
    Vector3 sunColor;

    // 3D Cloud parameters
    float cloudDensity;  // Overall cloud thickness/opacity (0.0-1.0)
    float cloudHeight;   // Cloud layer altitude (world units)
    float cloudScale;    // Cloud detail scale (larger = bigger clouds)
    float cloudSpeed;    // Animation speed multiplier
    float cloudCoverage; // Cloud coverage amount (0.0-1.0)
    Vector3 cloudOffset; // 3D offset for cloud animation

public:
    SkyboxRenderer();
    ~SkyboxRenderer();

    void Load(const char *vsPath, const char *fsPath);
    void SetSkyColor(Vector3 color);
    void SetCloudColor(Vector3 color);
    void SetSunDirection(Vector3 direction);
    void SetSunColor(Vector3 color);

    // 3D Cloud setters
    void SetCloudDensity(float density);
    void SetCloudHeight(float height);
    void SetCloudScale(float scale);
    void SetCloudSpeed(float speed);
    void SetCloudCoverage(float coverage);

    void Update(float deltaTime);
    void Draw(Camera3D camera);
    void Unload();
};
