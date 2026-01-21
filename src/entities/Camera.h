#pragma once
#include "raylib.h"
#include "raymath.h"

class CameraEntity : public Camera3D
{
public:
    CameraEntity();
    void Initialize(Vector3 position, Vector3 target, float fovy = 45.0f);

    // Desired target (controller sets these; entity applies smoothing in UpdateEntity)
    void SetDesired(const Vector3 &position, const Vector3 &target, float fovy);
    void SetPositionImmediate(const Vector3 &position);
    void SetTargetImmediate(const Vector3 &target);
    void SetFovImmediate(float fovy);
    void SetSmoothing(float smoothness);

    // Per-frame update to move camera towards desired state (smoothing applied here)
    void UpdateEntity(float deltaTime);

    // Utility math (shared with controllers)
    static Vector3 LerpVector3(const Vector3 &start, const Vector3 &end, float t);
    static float EaseInOutCubic(float t);

private:
    Vector3 m_DesiredPosition;
    Vector3 m_DesiredTarget;
    float m_DesiredFov;
    float m_Smoothing; // [0,1] lerp factor used each frame
};
