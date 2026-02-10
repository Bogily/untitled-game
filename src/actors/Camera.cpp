#include "Camera.h"

CameraEntity::CameraEntity()
    : m_DesiredPosition({0, 0, 0}), m_DesiredTarget({0, 0, 0}), m_DesiredFov(45.0f), m_Smoothing(0.1f)
{
    // Sensible defaults
    position = {0.0f, 10.0f, 10.0f};
    target = {0.0f, 0.0f, 0.0f};
    up = {0.0f, 1.0f, 0.0f};
    fovy = 45.0f;
    projection = CAMERA_PERSPECTIVE;

    // Desired initial state matches immediate state
    m_DesiredPosition = position;
    m_DesiredTarget = target;
    m_DesiredFov = fovy;
}

void CameraEntity::Initialize(Vector3 position_, Vector3 target_, float fovy_)
{
    position = position_;
    target = target_;
    up = {0.0f, 1.0f, 0.0f};
    fovy = fovy_;
    projection = CAMERA_PERSPECTIVE;

    m_DesiredPosition = position;
    m_DesiredTarget = target;
    m_DesiredFov = fovy;
}

void CameraEntity::SetDesired(const Vector3 &position_, const Vector3 &target_, float fovy_)
{
    m_DesiredPosition = position_;
    m_DesiredTarget = target_;
    m_DesiredFov = fovy_;
}

void CameraEntity::SetPositionImmediate(const Vector3 &position_)
{
    position = position_;
    m_DesiredPosition = position_;
}

void CameraEntity::SetTargetImmediate(const Vector3 &target_)
{
    target = target_;
    m_DesiredTarget = target_;
}

void CameraEntity::SetFovImmediate(float fovy_)
{
    fovy = fovy_;
    m_DesiredFov = fovy_;
}

void CameraEntity::SetSmoothing(float smoothness)
{
    m_Smoothing = smoothness;
}

void CameraEntity::UpdateEntity(float deltaTime)
{
    // Simple per-frame LERP smoothing towards desired values
    // Use fixed lerp factor each frame (camera smoothing is a small value like 0.1f)
    position = CameraEntity::LerpVector3(position, m_DesiredPosition, m_Smoothing);
    target = CameraEntity::LerpVector3(target, m_DesiredTarget, m_Smoothing);
    fovy = Lerp(fovy, m_DesiredFov, m_Smoothing);
}

Vector3 CameraEntity::LerpVector3(const Vector3 &start, const Vector3 &end, float t)
{
    return {
        Lerp(start.x, end.x, t),
        Lerp(start.y, end.y, t),
        Lerp(start.z, end.z, t)};
}

float CameraEntity::EaseInOutCubic(float t)
{
    if (t < 0.5f)
        return 4.0f * t * t * t;
    else
    {
        float f = (2.0f * t - 2.0f);
        return 0.5f * f * f * f + 1.0f;
    }
}
