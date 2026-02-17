#include "Camera.h"

CameraEntity::CameraEntity()
    : desiredPosition({0, 0, 0}), desiredTarget({0, 0, 0}), desiredFov(45.0f), smoothing(0.1f)
{
    // Sensible defaults
    position = {0.0f, 10.0f, 10.0f};
    target = {0.0f, 0.0f, 0.0f};
    up = {0.0f, 1.0f, 0.0f};
    fovy = 45.0f;
    projection = CAMERA_PERSPECTIVE;

    // Desired initial state matches immediate state
    desiredPosition = position;
    desiredTarget = target;
    desiredFov = fovy;
}

void CameraEntity::Initialize(Vector3 position_, Vector3 target_, float fovy_)
{
    position = position_;
    target = target_;
    up = {0.0f, 1.0f, 0.0f};
    fovy = fovy_;
    projection = CAMERA_PERSPECTIVE;

    desiredPosition = position;
    desiredTarget = target;
    desiredFov = fovy;
}

void CameraEntity::SetDesired(const Vector3 &position_, const Vector3 &target_, float fovy_)
{
    desiredPosition = position_;
    desiredTarget = target_;
    desiredFov = fovy_;
}

void CameraEntity::SetPositionImmediate(const Vector3 &position_)
{
    position = position_;
    desiredPosition = position_;
}

void CameraEntity::SetTargetImmediate(const Vector3 &target_)
{
    target = target_;
    desiredTarget = target_;
}

void CameraEntity::SetFovImmediate(float fovy_)
{
    fovy = fovy_;
    desiredFov = fovy_;
}

void CameraEntity::SetSmoothing(float smoothness)
{
    smoothing = smoothness;
}

void CameraEntity::UpdateEntity(float deltaTime)
{
    (void)deltaTime;

    if (smoothing <= 0.0f)
    {
        position = desiredPosition;
        target = desiredTarget;
        fovy = desiredFov;
        return;
    }

    // Simple per-frame LERP smoothing towards desired values
    // Use fixed lerp factor each frame (camera smoothing is a small value like 0.1f)
    position = CameraEntity::LerpVector3(position, desiredPosition, smoothing);
    target = CameraEntity::LerpVector3(target, desiredTarget, smoothing);
    fovy = Lerp(fovy, desiredFov, smoothing);
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
