#pragma once

#include <raylib.h>
#include <raymath.h>

class WaterRenderer
{
public:
    WaterRenderer();
    ~WaterRenderer();

    void Init();
    void Update(float deltaTime, Camera3D camera);
    void Draw();
    void Cleanup();

    void SetLightDirection(Vector3 direction);
    void SetWaterLevel(float yPosition);
    void SetWaterSize(float width, float depth);
    void SetUseComputeShader(bool enable);

private:
    void InitComputeShader();
    void RunComputeShader(float deltaTime);
    void UpdateMeshFromCompute();

    Shader waterShader;
    Mesh waterMesh;
    Model waterModel;
    Material waterMaterial;

    float waterLevel;
    float waterWidth;
    float waterDepth;
    float elapsedTime;

    Vector3 lightDirection;

    // Shader uniform locations
    int timeLoc;
    int viewPosLoc;
    int lightDirLoc;
    int mvpLoc;
    int matModelLoc;
    int matNormalLoc;
    int normalScaleLoc;
    int foamThresholdLoc;
    int foamIntensityLoc;
    int glossinessLoc;

    // Compute shader for water simulation
    unsigned int computeProgram;
    unsigned int ssboHeightField;
    unsigned int ssboVelocityField;
    unsigned int ssboPrevHeightField;

    int gridWidth;
    int gridHeight;
    bool useComputeShader;

    // Compute shader uniform locations
    int compDeltaTimeLoc;
    int compTimeLoc;
    int compGridWidthLoc;
    int compGridHeightLoc;
    int compDampingLoc;
    int compWaveSpeedLoc;
    int compWaveStrengthLoc;
    int compWindDirectionLoc;
    int compWindStrengthLoc;

    float damping;
    float waveSpeed;
    float waveStrength;
    Vector2 windDirection;
    float windStrength;

    // Visual tweak uniforms
    float normalScale;
    float foamThreshold;
    float foamIntensity;
    float glossiness;

    bool initialized;
};
