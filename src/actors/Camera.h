/**
 * @file Camera.h
 * @brief Enhanced camera entity with smoothing
 */

#pragma once
#include "raylib.h"
#include "raymath.h"

/**
 * @brief Camera entity with smooth interpolation
 *
 * Extends raylib Camera3D with smooth movement and position interpolation.
 * Controllers set desired position/target, entity applies smoothing over time.
 */
class CameraEntity : public Camera3D
{
public:
    /**
     * @brief Construct a new camera entity
     */
    CameraEntity();

    /**
     * @brief Initialize camera with starting parameters
     * @param position Initial camera position
     * @param target Initial look-at target
     * @param fovy Field of view Y (default 45.0f)
     */
    void Initialize(Vector3 position, Vector3 target, float fovy = 45.0f);

    /**
     * @brief Set desired camera state (smoothing will be applied)
     * @param position Target position
     * @param target Target look-at point
     * @param fovy Target field of view
     */
    void SetDesired(const Vector3 &position, const Vector3 &target, float fovy);

    /**
     * @brief Set position immediately without smoothing
     * @param position New camera position
     */
    void SetPositionImmediate(const Vector3 &position);

    /**
     * @brief Set target immediately without smoothing
     * @param target New look-at target
     */
    void SetTargetImmediate(const Vector3 &target);

    /**
     * @brief Set field of view immediately without smoothing
     * @param fovy New field of view Y
     */
    void SetFovImmediate(float fovy);

    /**
     * @brief Set smoothing factor
     * @param smoothness Lerp factor [0,1], higher = smoother
     */
    void SetSmoothing(float smoothness);

    /**
     * @brief Update camera position towards desired state
     * @param deltaTime Time elapsed since last frame
     */
    void UpdateEntity(float deltaTime);

    /**
     * @brief Linearly interpolate between two vectors
     * @param start Start vector
     * @param end End vector
     * @param t Interpolation factor [0,1]
     * @return Interpolated vector
     */
    static Vector3 LerpVector3(const Vector3 &start, const Vector3 &end, float t);

    /**
     * @brief Ease-in-out cubic interpolation
     * @param t Input value [0,1]
     * @return Eased value [0,1]
     */
    static float EaseInOutCubic(float t);

private:
    Vector3 m_DesiredPosition; ///< Target position for smooth movement
    Vector3 m_DesiredTarget;   ///< Target look-at point for smooth movement
    float m_DesiredFov;        ///< Target field of view
    float m_Smoothing;         ///< Lerp factor for smoothing [0,1]
};
