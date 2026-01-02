#pragma once
#include "raylib.h"

class Skybox
{
private:
    Shader shader;
    Model cube;
    int timeLoc;
    int skyColorLoc;
    int cloudColorLoc;
    int sunDirectionLoc;
    int sunColorLoc;
    float time;
    Vector3 skyColor;
    Vector3 cloudColor;
    Vector3 sunDirection;
    Vector3 sunColor;

public:
    Skybox();
    ~Skybox();

    void Load(const char *vsPath, const char *fsPath);
    void SetSkyColor(Vector3 color);
    void SetCloudColor(Vector3 color);
    void SetSunDirection(Vector3 direction);
    void SetSunColor(Vector3 color);
    void Update(float deltaTime);
    void Draw(Camera3D camera);
    void Unload();
};
