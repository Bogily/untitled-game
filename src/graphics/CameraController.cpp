#include "CameraController.h"
#include <cmath>

CameraController::CameraController()
    : mode(CAMERA_MODE_FREE),
      followTargetPtr(nullptr),
      followDistance(10.0f),
      followHeight(5.0f),
      followYaw(0.0f),
      followPitch(-20.0f),
      cameraSmoothness(0.1f),
      currentWaypointIndex(0),
      cutsceneTimer(0.0f),
      isPlayingCutscene(false),
      isTransitioning(false),
      transitionTimer(0.0f),
      transitionDuration(0.0f)
{
    camera = {0};
    camera.position = (Vector3){0.0f, 10.0f, 10.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

void CameraController::Initialize(Vector3 position, Vector3 target, float fovy)
{
    camera.position = position;
    camera.target = target;
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = fovy;
    camera.projection = CAMERA_PERSPECTIVE;
}

void CameraController::Update(float deltaTime)
{
    // Handle smooth transitions first
    if (isTransitioning)
    {
        transitionTimer += deltaTime;
        float t = transitionTimer / transitionDuration;

        if (t >= 1.0f)
        {
            // Transition complete
            camera.position = transitionEndPos;
            camera.target = transitionEndTarget;
            isTransitioning = false;
        }
        else
        {
            // Apply easing
            float easedT = EaseInOutCubic(t);
            camera.position = LerpVector3(transitionStartPos, transitionEndPos, easedT);
            camera.target = LerpVector3(transitionStartTarget, transitionEndTarget, easedT);
        }
        return;
    }

    // Update based on current mode
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
}

void CameraController::UpdateFreeCamera(float deltaTime)
{
    // Basic free camera controls (can be expanded)
    // This is just a placeholder - you might want to use Raylib's UpdateCamera
    // or implement custom mouse/keyboard controls here
}

void CameraController::UpdateFollowCamera(Vector3 targetPosition, float deltaTime)
{
    // Calculate desired camera position behind and above the target
    float yawRad = followYaw * DEG2RAD;
    float pitchRad = followPitch * DEG2RAD;

    Vector3 offset;
    offset.x = followDistance * cosf(pitchRad) * sinf(yawRad);
    offset.y = followDistance * sinf(pitchRad) + followHeight;
    offset.z = followDistance * cosf(pitchRad) * cosf(yawRad);

    Vector3 desiredPosition = Vector3Add(targetPosition, offset);
    Vector3 desiredTarget = Vector3Add(targetPosition, (Vector3){0.0f, followHeight * 0.5f, 0.0f});

    // Smooth interpolation
    camera.position = LerpVector3(camera.position, desiredPosition, cameraSmoothness);
    camera.target = LerpVector3(camera.target, desiredTarget, cameraSmoothness);
}

void CameraController::UpdateCutsceneCamera(float deltaTime)
{
    if (!isPlayingCutscene || cutsceneWaypoints.empty())
    {
        StopCutscene();
        return;
    }

    cutsceneTimer += deltaTime;

    // Check if we need to move to next waypoint
    if (currentWaypointIndex >= cutsceneWaypoints.size())
    {
        // Cutscene finished
        StopCutscene();
        return;
    }

    const CameraWaypoint &currentWaypoint = cutsceneWaypoints[currentWaypointIndex];

    if (cutsceneTimer >= currentWaypoint.duration)
    {
        // Move to next waypoint
        currentWaypointIndex++;
        cutsceneTimer = 0.0f;

        if (currentWaypointIndex >= cutsceneWaypoints.size())
        {
            // Reached the end
            camera.position = currentWaypoint.position;
            camera.target = currentWaypoint.target;
            camera.fovy = currentWaypoint.fov;
            return;
        }
    }

    // Interpolate between current and next waypoint
    if (currentWaypointIndex < cutsceneWaypoints.size())
    {
        const CameraWaypoint &waypoint = cutsceneWaypoints[currentWaypointIndex];
        float t = cutsceneTimer / waypoint.duration;
        t = EaseInOutCubic(t);

        // Get start position (previous waypoint or current position)
        Vector3 startPos = (currentWaypointIndex == 0) ? camera.position : cutsceneWaypoints[currentWaypointIndex - 1].position;
        Vector3 startTarget = (currentWaypointIndex == 0) ? camera.target : cutsceneWaypoints[currentWaypointIndex - 1].target;
        float startFov = (currentWaypointIndex == 0) ? camera.fovy : cutsceneWaypoints[currentWaypointIndex - 1].fov;

        // Interpolate
        camera.position = LerpVector3(startPos, waypoint.position, t);
        camera.target = LerpVector3(startTarget, waypoint.target, t);
        camera.fovy = Lerp(startFov, waypoint.fov, t);
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

    // Return to follow mode or free mode
    mode = CAMERA_MODE_FREE;
}

void CameraController::SetMode(CameraControllerMode newMode)
{
    mode = newMode;

    // Reset mode-specific data
    if (mode != CAMERA_MODE_CUTSCENE)
    {
        isPlayingCutscene = false;
        cutsceneWaypoints.clear();
    }
}

void CameraController::SetPosition(Vector3 position)
{
    camera.position = position;
}

void CameraController::SetTarget(Vector3 target)
{
    camera.target = target;
}

void CameraController::SetFollowTarget(Vector3 *targetPtr)
{
    followTargetPtr = targetPtr;
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

    // Don't update follow/cutscene modes during transition
    if (mode == CAMERA_MODE_FOLLOW || mode == CAMERA_MODE_CUTSCENE)
    {
        mode = CAMERA_MODE_FIXED;
    }
}

Vector3 CameraController::LerpVector3(Vector3 start, Vector3 end, float t)
{
    return (Vector3){
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
