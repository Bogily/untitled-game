#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

enum CameraControllerMode
{
    CAMERA_MODE_FREE,     // Free camera (manual control)
    CAMERA_MODE_FOLLOW,   // Follow a target (player)
    CAMERA_MODE_CUTSCENE, // Cutscene mode with waypoints
    CAMERA_MODE_FIXED     // Fixed camera position
};

struct CameraWaypoint
{
    Vector3 position;
    Vector3 target;
    float duration; // Time to reach this waypoint
    float fov;      // Field of view at this waypoint
};

class CameraController
{
public:
    Camera3D camera;
    CameraControllerMode mode;

    // Constructor
    CameraController();

    // Initialize camera with default position
    void Initialize(Vector3 position, Vector3 target, float fovy = 45.0f);

    // Update camera based on current mode
    void Update(float deltaTime);

    // Mode-specific updates
    void UpdateFreeCamera(float deltaTime);
    void UpdateFollowCamera(Vector3 targetPosition, float deltaTime);
    void UpdateCutsceneCamera(float deltaTime);

    // Cutscene controls
    void StartCutscene(const std::vector<CameraWaypoint> &waypoints);
    void StopCutscene();
    bool IsCutscenePlaying() const { return mode == CAMERA_MODE_CUTSCENE && isPlayingCutscene; }

    // Camera controls
    void SetMode(CameraControllerMode newMode);
    void SetPosition(Vector3 position);
    void SetTarget(Vector3 target);
    void SetFollowTarget(Vector3 *targetPtr);
    void SetFollowDistance(float distance) { followDistance = distance; }
    void SetFollowHeight(float height) { followHeight = height; }
    void SetSmoothness(float smoothness) { cameraSmoothness = smoothness; }

    // Smooth transitions
    void TransitionTo(Vector3 newPosition, Vector3 newTarget, float duration);
    bool IsTransitioning() const { return isTransitioning; }

private:
    // Follow mode settings
    Vector3 *followTargetPtr;
    float followDistance;
    float followHeight;
    float followYaw;
    float followPitch;
    float cameraSmoothness;

    // Cutscene data
    std::vector<CameraWaypoint> cutsceneWaypoints;
    int currentWaypointIndex;
    float cutsceneTimer;
    bool isPlayingCutscene;

    // Transition data
    bool isTransitioning;
    float transitionTimer;
    float transitionDuration;
    Vector3 transitionStartPos;
    Vector3 transitionEndPos;
    Vector3 transitionStartTarget;
    Vector3 transitionEndTarget;

    // Helper functions
    Vector3 LerpVector3(Vector3 start, Vector3 end, float t);
    float EaseInOutCubic(float t);
};
