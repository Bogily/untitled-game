/**
 * @file CameraController.h
 * @brief Camera controller with multiple control modes
 */

#pragma once
#include "raylib.h"
#include "raymath.h"
#include "actors/Camera.h"
#include <vector>

/**
 * @brief Camera control modes
 */
enum CameraControllerMode
{
    CAMERA_MODE_FREE,     ///< Free camera with manual control
    CAMERA_MODE_FOLLOW,   ///< Follow a target position (player)
    CAMERA_MODE_CUTSCENE, ///< Cutscene mode with waypoints
    CAMERA_MODE_FIXED     ///< Fixed camera position
};

/**
 * @brief Camera waypoint for cutscene paths
 */
struct CameraWaypoint
{
    Vector3 position; ///< Camera position
    Vector3 target;   ///< Look-at target
    float duration;   ///< Time to reach this waypoint (seconds)
    float fov;        ///< Field of view at waypoint (degrees)
};

class CameraController
{
public:
    CameraEntity camera;
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
    void SetSmoothness(float smoothness);

    // Free camera controls
    void SetFreeCameraSpeed(float speed) { freeCameraSpeed = speed; }
    void SetFreeCameraMouseSensitivity(float sensitivity) { freeCameraMouseSensitivity = sensitivity; }
    float GetFreeCameraSpeed() const { return freeCameraSpeed; }
    float GetFreeCameraMouseSensitivity() const { return freeCameraMouseSensitivity; }
    float GetFreeCameraYaw() const { return freeCameraYaw; }
    float GetFreeCameraPitch() const { return freeCameraPitch; }

    // Smooth transitions
    void TransitionTo(Vector3 newPosition, Vector3 newTarget, float duration);
    bool IsTransitioning() const { return isTransitioning; }

private:
    Vector3 *followTargetPtr; ///< Pointer to follow target
    float followDistance;     ///< Distance from target
    float followHeight;       ///< Height offset from target
    float followYaw;          ///< Follow camera yaw angle
    float followPitch;        ///< Follow camera pitch angle
    float cameraSmoothness;   ///< Movement smoothness factor

    // Free camera parameters
    float freeCameraSpeed = 10.0f;             ///< Free camera movement speed
    float freeCameraMouseSensitivity = 0.003f; ///< Mouse look sensitivity
    float freeCameraYaw = 0.0f;                ///< Free camera yaw angle
    float freeCameraPitch = 0.0f;              ///< Free camera pitch angle

    std::vector<CameraWaypoint> cutsceneWaypoints; ///< Cutscene waypoint path
    int currentWaypointIndex;                      ///< Current waypoint index
    float cutsceneTimer;                           ///< Cutscene time accumulator
    bool isPlayingCutscene;                        ///< Cutscene active flag

    bool isTransitioning;          ///< Transition active flag
    float transitionTimer;         ///< Transition time accumulator
    float transitionDuration;      ///< Transition total duration
    Vector3 transitionStartPos;    ///< Transition start position
    Vector3 transitionEndPos;      ///< Transition end position
    Vector3 transitionStartTarget; ///< Transition start target
    Vector3 transitionEndTarget;   ///< Transition end target

    /**
     * @brief Linearly interpolate between vectors
     * @param start Start vector
     * @param end End vector
     * @param t Interpolation factor [0,1]
     * @return Interpolated vector
     */
    Vector3 LerpVector3(Vector3 start, Vector3 end, float t);

    /**
     * @brief Ease-in-out cubic interpolation
     * @param t Input value [0,1]
     * @return Smoothed value [0,1]
     */
    float EaseInOutCubic(float t);
};
