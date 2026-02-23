#include "CameraController.h"
#include <cmath>

CameraController::CameraController()
    : mode(CAMERA_MODE_FREE),
      followTargetPtr(nullptr),
      followDistance(10.0f),
      followHeight(5.0f),
      followEyeHeight(1.6f),
      followAutoMoveSmoothSpeed(12.0f),
      followYaw(0.0f),
      followPitch(-20.0f),
      cutsceneSmoothingFactor(0.1f),
      cutsceneSmoothingEnabled(false),
      freeCameraSpeed(10.0f),
      freeCameraMouseSensitivity(0.0003f),
      freeCameraYaw(0.0f),
      freeCameraPitch(0.0f),
      currentWaypointIndex(0),
      cutsceneTimer(0.0f),
      isPlayingCutscene(false),
      isTransitioning(false),
      transitionTimer(0.0f),
      transitionDuration(0.0f)
{
    camera.SetSmoothing(0.0f);
}

void CameraController::Initialize(Vector3 position, Vector3 target, float fovy)
{
    camera.Initialize(position, target, fovy);
    camera.SetSmoothing(0.0f);
}

void CameraController::Update(float deltaTime)
{
    const float activeSmoothing = (mode == CAMERA_MODE_CUTSCENE && cutsceneSmoothingEnabled)
                                      ? cutsceneSmoothingFactor
                                      : 0.0f;
    camera.SetSmoothing(activeSmoothing);

    if (isTransitioning)
    {
        transitionTimer += deltaTime;
        float t = transitionTimer / transitionDuration;

        if (t >= 1.0f)
        {
            camera.SetPositionImmediate(transitionEndPos);
            camera.SetTargetImmediate(transitionEndTarget);
            isTransitioning = false;
        }
        else
        {
            float easedT = CameraEntity::EaseInOutCubic(t);
            Vector3 pos = CameraEntity::LerpVector3(transitionStartPos, transitionEndPos, easedT);
            Vector3 tgt = CameraEntity::LerpVector3(transitionStartTarget, transitionEndTarget, easedT);
            camera.SetDesired(pos, tgt, camera.fovy);
        }
        return;
    }

    switch (mode)
    {
    case CAMERA_MODE_FREE:
        UpdateFreeCamera(deltaTime);
        break;

    case CAMERA_MODE_FOLLOW:
        if (followTargetPtr != nullptr)
            UpdateFollowCamera(*followTargetPtr, deltaTime);
        break;

    case CAMERA_MODE_CUTSCENE:
        UpdateCutsceneCamera(deltaTime);
        break;

    case CAMERA_MODE_FIXED:
        // Do nothing - camera is fixed
        break;
    }

    if (shakeIntensity > 0.0f && shakeDuration > 0.0f)
    {
        shakeTimer += deltaTime;
        float shakeProgress = shakeTimer / shakeDuration;

        if (shakeProgress >= 1.0f)
        {
            shakeIntensity = 0.0f;
            shakeTimer = 0.0f;
        }
        else
        {
            float easeOut = 1.0f - (shakeProgress * shakeProgress);
            float currentIntensity = shakeIntensity * easeOut;

            Vector3 shakeOffset;
            shakeOffset.x = (GetRandomValue(-100, 100) / 100.0f) * currentIntensity;
            shakeOffset.y = (GetRandomValue(-100, 100) / 100.0f) * currentIntensity;
            shakeOffset.z = (GetRandomValue(-100, 100) / 100.0f) * currentIntensity;

            camera.position = Vector3Add(camera.position, shakeOffset);
        }
    }
}

void CameraController::UpdateFreeCamera(float deltaTime)
{
    Vector2 mouseDelta = GetMouseDelta();

    if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f)
    {
        freeCameraYaw -= mouseDelta.x * freeCameraMouseSensitivity * 100.0f;
        freeCameraPitch -= mouseDelta.y * freeCameraMouseSensitivity * 100.0f;

        if (freeCameraPitch > 89.0f)
            freeCameraPitch = 89.0f;
        if (freeCameraPitch < -89.0f)
            freeCameraPitch = -89.0f;
    }

    float yawRad = freeCameraYaw * DEG2RAD;
    float pitchRad = freeCameraPitch * DEG2RAD;

    Vector3 forward;
    forward.x = sinf(yawRad) * cosf(pitchRad);
    forward.y = sinf(pitchRad);
    forward.z = cosf(yawRad) * cosf(pitchRad);
    forward = Vector3Normalize(forward);

    Vector3 right = Vector3CrossProduct(forward, {0.0f, 1.0f, 0.0f});
    right = Vector3Normalize(right);

    Vector3 up = Vector3CrossProduct(right, forward);
    up = Vector3Normalize(up);

    Vector3 movement = {0.0f, 0.0f, 0.0f};

    if (IsKeyDown(KEY_W))
        movement = Vector3Add(movement, Vector3Scale(forward, freeCameraSpeed * deltaTime));
    if (IsKeyDown(KEY_S))
        movement = Vector3Add(movement, Vector3Scale(forward, -freeCameraSpeed * deltaTime));
    if (IsKeyDown(KEY_D))
        movement = Vector3Add(movement, Vector3Scale(right, freeCameraSpeed * deltaTime));
    if (IsKeyDown(KEY_A))
        movement = Vector3Add(movement, Vector3Scale(right, -freeCameraSpeed * deltaTime));
    if (IsKeyDown(KEY_SPACE))
        movement = Vector3Add(movement, Vector3Scale(up, freeCameraSpeed * deltaTime));
    if (IsKeyDown(KEY_LEFT_CONTROL))
        movement = Vector3Add(movement, Vector3Scale(up, -freeCameraSpeed * deltaTime));

    Vector3 newPosition = Vector3Add(camera.position, movement);

    Vector3 newTarget = Vector3Add(newPosition, forward);

    camera.SetDesired(newPosition, newTarget, camera.fovy);
}

void CameraController::UpdateFollowCamera(Vector3 targetPosition, float deltaTime)
{
    Vector2 mouseDelta = GetMouseDelta();
    if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f)
    {
        followYaw -= mouseDelta.x * freeCameraMouseSensitivity * 100.0f;
        followPitch -= mouseDelta.y * freeCameraMouseSensitivity * 100.0f;

        if (followPitch > 60.0f)
            followPitch = 60.0f;
        if (followPitch < -80.0f)
            followPitch = -80.0f;
    }

    float yawRad = followYaw * DEG2RAD;
    float pitchRad = followPitch * DEG2RAD;

    Vector3 eyePosition = Vector3Add(targetPosition, {0.0f, followEyeHeight, 0.0f});

    Vector3 offset;
    offset.x = followDistance * cosf(pitchRad) * sinf(yawRad);
    offset.y = followDistance * sinf(pitchRad);
    offset.z = followDistance * cosf(pitchRad) * cosf(yawRad);

    Vector3 desiredPosition = Vector3Add(eyePosition, offset);
    Vector3 desiredTarget = eyePosition;
    Vector3 lookDirection = Vector3Normalize(Vector3Subtract(desiredTarget, desiredPosition));
    if (Vector3Length(lookDirection) <= 1e-4f)
        lookDirection = {0.0f, 0.0f, -1.0f};

    Vector3 orbitVector = Vector3Subtract(desiredPosition, eyePosition);
    float orbitLength = Vector3Length(orbitVector);
    if (orbitLength <= 1e-4f)
    {
        orbitVector = {0.0f, 0.0f, 1.0f};
        orbitLength = 1.0f;
    }
    Vector3 orbitDirection = Vector3Scale(orbitVector, 1.0f / orbitLength);
    float desiredDistance = orbitLength;

    if (followCollisionRaycast)
    {
        float rayLength = desiredDistance;
        if (rayLength > 1e-4f)
        {
            Ray ray = {};
            ray.position = eyePosition;
            ray.direction = orbitDirection;

            Vector3 hitPosition = desiredPosition;
            if (followCollisionRaycast(ray, rayLength, hitPosition))
            {
                desiredDistance = Vector3Distance(eyePosition, hitPosition);
            }
        }
    }

    const float minCameraDistance = 0.35f;
    if (desiredDistance < minCameraDistance)
        desiredDistance = minCameraDistance;

    float currentDistance = Vector3DotProduct(Vector3Subtract(camera.position, eyePosition), orbitDirection);
    if (currentDistance < minCameraDistance)
        currentDistance = minCameraDistance;

    float blend = Clamp(deltaTime * followAutoMoveSmoothSpeed, 0.0f, 1.0f);
    float smoothedDistance = Lerp(currentDistance, desiredDistance, blend);
    desiredPosition = Vector3Add(eyePosition, Vector3Scale(orbitDirection, smoothedDistance));
    desiredTarget = Vector3Add(desiredPosition, Vector3Scale(lookDirection, followDistance));

    camera.SetDesired(desiredPosition, desiredTarget, camera.fovy);
}

void CameraController::UpdateCutsceneCamera(float deltaTime)
{
    if (!isPlayingCutscene || cutsceneWaypoints.empty())
    {
        StopCutscene();
        return;
    }

    cutsceneTimer += deltaTime;

    if (currentWaypointIndex >= cutsceneWaypoints.size())
    {
        StopCutscene();
        return;
    }

    const CameraWaypoint &currentWaypoint = cutsceneWaypoints[currentWaypointIndex];

    if (cutsceneTimer >= currentWaypoint.duration)
    {
        currentWaypointIndex++;
        cutsceneTimer = 0.0f;

        if (currentWaypointIndex >= cutsceneWaypoints.size())
        {
            camera.SetPositionImmediate(currentWaypoint.position);
            camera.SetTargetImmediate(currentWaypoint.target);
            camera.SetFovImmediate(currentWaypoint.fov);
            return;
        }
    }

    if (currentWaypointIndex < cutsceneWaypoints.size())
    {
        const CameraWaypoint &waypoint = cutsceneWaypoints[currentWaypointIndex];
        float t = cutsceneTimer / waypoint.duration;
        t = CameraEntity::EaseInOutCubic(t);

        Vector3 startPos = (currentWaypointIndex == 0) ? camera.position : cutsceneWaypoints[currentWaypointIndex - 1].position;
        Vector3 startTarget = (currentWaypointIndex == 0) ? camera.target : cutsceneWaypoints[currentWaypointIndex - 1].target;
        float startFov = (currentWaypointIndex == 0) ? camera.fovy : cutsceneWaypoints[currentWaypointIndex - 1].fov;

        Vector3 interpPos = CameraEntity::LerpVector3(startPos, waypoint.position, t);
        Vector3 interpTgt = CameraEntity::LerpVector3(startTarget, waypoint.target, t);
        float interpFov = Lerp(startFov, waypoint.fov, t);
        camera.SetDesired(interpPos, interpTgt, interpFov);
    }
}

void CameraController::StartCutscene(const std::vector<CameraWaypoint> &waypoints)
{
    if (waypoints.empty())
        return;

    cutsceneWaypoints = waypoints;
    currentWaypointIndex = 0;
    cutsceneTimer = 0.0f;
    isPlayingCutscene = true;
    mode = CAMERA_MODE_CUTSCENE;
}

void CameraController::StopCutscene()
{
    isPlayingCutscene = false;
    cutsceneWaypoints.clear();
    currentWaypointIndex = 0;
    cutsceneTimer = 0.0f;

    mode = CAMERA_MODE_FREE;
}

void CameraController::ApplyShake(float intensity, float duration)
{
    shakeIntensity = fmaxf(0.0f, fminf(1.0f, intensity));
    shakeDuration = fmaxf(0.1f, duration);
    shakeTimer = 0.0f;
}

void CameraController::SetMode(CameraControllerMode newMode)
{
    mode = newMode;

    if (mode != CAMERA_MODE_CUTSCENE)
    {
        isPlayingCutscene = false;
        cutsceneWaypoints.clear();
    }
}

void CameraController::SetPosition(Vector3 position)
{
    camera.SetPositionImmediate(position);
}

void CameraController::SetTarget(Vector3 target)
{
    camera.SetTargetImmediate(target);
}

void CameraController::SetFollowTarget(Vector3 *targetPtr)
{
    followTargetPtr = targetPtr;
}

void CameraController::SetCutsceneSmoothingFactor(float smoothingFactor)
{
    cutsceneSmoothingFactor = fmaxf(0.0f, fminf(1.0f, smoothingFactor));
}

void CameraController::TransitionTo(Vector3 newPosition, Vector3 newTarget, float duration)
{
    isTransitioning = true;
    transitionTimer = 0.0f;
    transitionDuration = duration;
    transitionStartPos = camera.position;
    transitionEndPos = newPosition;
    transitionStartTarget = camera.target;
    transitionEndTarget = newTarget;

    if (mode == CAMERA_MODE_FOLLOW || mode == CAMERA_MODE_CUTSCENE)
    {
        mode = CAMERA_MODE_FIXED;
    }
}

Vector3 CameraController::LerpVector3(Vector3 start, Vector3 end, float t)
{
    return {
        Lerp(start.x, end.x, t),
        Lerp(start.y, end.y, t),
        Lerp(start.z, end.z, t)};
}

float CameraController::EaseInOutCubic(float t)
{
    if (t < 0.5f)
        return 4.0f * t * t * t;
    else
    {
        float f = (2.0f * t - 2.0f);
        return 0.5f * f * f * f + 1.0f;
    }
}
