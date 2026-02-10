/**
 * @file Player.h
 * @brief Player entity and control system
 */

#pragma once
#include "Actor.h"
#include "raylib.h"
#include <vector>

/**
 * @brief Player character entity
 *
 * Manages player position, rotation, model, and provides
 * raycasting for interactions.
 */
class Player : public Actor
{
public:
    float playerYaw = 0.0f;        ///< Player body rotation (Y-axis)
    float cameraYaw = 0.0f;        ///< Camera horizontal rotation
    float cameraPitch = -30.0f;    ///< Camera vertical rotation
    float sprintMultiplier = 2.0f; ///< Sprint speed multiplier
    float eyeHeight = 1.6f;        ///< Eye/camera height offset from feet

    /**
     * @brief Update player state
     */
    void Update();

    /**
     * @brief Render player model
     */
    void Draw();

    /**
     * @brief Get forward-facing ray from player position
     * @return Ray originating from player eye position
     */
    Ray GetForwardRay() const;
};
