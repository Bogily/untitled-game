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
      freeCameraSpeed(10.0f),
      freeCameraMouseSensitivity(0.003f),
      freeCameraYaw(0.0f),
      freeCameraPitch(0.0f),
      currentWaypointIndex(0),
      cutsceneTimer(0.0f),
      isPlayingCutscene(false),
      isTransitioning(false),
      transitionTimer(0.0f),
      transitionDuration(0.0f)
{
    // CameraEntity constructor sets sensible defaults; ensure smoothing is set
    camera.SetSmoothing(cameraSmoothness);
}

void CameraController::Initialize(Vector3 position, Vector3 target, float fovy)
{
    camera.Initialize(position, target, fovy);
    camera.SetSmoothing(cameraSmoothness);
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
            // Transition complete (set immediate)
            camera.SetPositionImmediate(transitionEndPos);
            camera.SetTargetImmediate(transitionEndTarget);
            isTransitioning = false;
        }
        else
        {
            // Apply easing and set desired position => entity will smooth towards it
            float easedT = CameraEntity::EaseInOutCubic(t);
            Vector3 pos = CameraEntity::LerpVector3(transitionStartPos, transitionEndPos, easedT);
            Vector3 tgt = CameraEntity::LerpVector3(transitionStartTarget, transitionEndTarget, easedT);
            camera.SetDesired(pos, tgt, camera.fovy);
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
    // Handle mouse input for camera rotation
    Vector2 mouseDelta = GetMouseDelta();

    if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f)
    {
        // Update yaw and pitch based on mouse movement (negate yaw for correct direction)
        freeCameraYaw -= mouseDelta.x * freeCameraMouseSensitivity * 100.0f;
        freeCameraPitch -= mouseDelta.y * freeCameraMouseSensitivity * 100.0f;

        // Clamp pitch to prevent camera flipping (+-89 degrees)
        if (freeCameraPitch > 89.0f)
            freeCameraPitch = 89.0f;
        if (freeCameraPitch < -89.0f)
            freeCameraPitch = -89.0f;
    }

    // Convert angles to radians
    float yawRad = freeCameraYaw * DEG2RAD;
    float pitchRad = freeCameraPitch * DEG2RAD;

    // Calculate forward, right, and up vectors
    Vector3 forward;
    forward.x = sinf(yawRad) * cosf(pitchRad);
    forward.y = sinf(pitchRad);
    forward.z = cosf(yawRad) * cosf(pitchRad);
    forward = Vector3Normalize(forward);

    Vector3 right = Vector3CrossProduct(forward, {0.0f, 1.0f, 0.0f});
    right = Vector3Normalize(right);

    Vector3 up = Vector3CrossProduct(right, forward);
    up = Vector3Normalize(up);

    // Handle keyboard input for movement
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

    // Apply movement to camera position
    Vector3 newPosition = Vector3Add(camera.position, movement);

    // Calculate new target (looking direction)
    Vector3 newTarget = Vector3Add(newPosition, forward);

    // Update camera
    camera.SetDesired(newPosition, newTarget, camera.fovy);
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
    Vector3 desiredTarget = Vector3Add(targetPosition, {0.0f, followHeight * 0.5f, 0.0f});

    // Set desired transform on the Camera entity; entity will apply smoothing
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
            // Reached the end - set immediate
            camera.SetPositionImmediate(currentWaypoint.position);
            camera.SetTargetImmediate(currentWaypoint.target);
            camera.SetFovImmediate(currentWaypoint.fov);
            return;
        }
    }

    // Interpolate between current and next waypoint
    if (currentWaypointIndex < cutsceneWaypoints.size())
    {
        const CameraWaypoint &waypoint = cutsceneWaypoints[currentWaypointIndex];
        float t = cutsceneTimer / waypoint.duration;
        t = CameraEntity::EaseInOutCubic(t);

        // Get start position (previous waypoint or current position)
        Vector3 startPos = (currentWaypointIndex == 0) ? camera.position : cutsceneWaypoints[currentWaypointIndex - 1].position;
        Vector3 startTarget = (currentWaypointIndex == 0) ? camera.target : cutsceneWaypoints[currentWaypointIndex - 1].target;
        float startFov = (currentWaypointIndex == 0) ? camera.fovy : cutsceneWaypoints[currentWaypointIndex - 1].fov;

        // Interpolate and set desired on the entity
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

void CameraController::SetSmoothness(float smoothness)
{
    cameraSmoothness = smoothness;
    camera.SetSmoothing(cameraSmoothness);
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
