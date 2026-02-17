/**
 * @file Frustum.h
 * @brief View frustum structures for culling
 */

#pragma once
#include "raylib.h"

/**
 * @brief Frustum plane for culling tests
 */
struct FrustumPlane
{
    Vector3 normal; ///< Plane normal vector
    float distance; ///< Distance from origin
};

/**
 * @brief View frustum for visibility culling
 *
 * Contains six planes defining the camera view volume.
 */
struct Frustum
{
    FrustumPlane planes[6]; ///< Near, Far, Left, Right, Top, Bottom
};

/**
 * @brief Build a frustum from a camera using the project's shared conventions
 */
Frustum BuildFrustumFromCamera(const Camera3D &camera);

/**
 * @brief Set the current global frustum cache
 */
void SetGlobalFrustum(const Frustum &frustum);

/**
 * @brief Recompute and update the global frustum from camera
 */
void UpdateGlobalFrustum(const Camera3D &camera);

/**
 * @brief Get current global frustum cache
 */
const Frustum &GetGlobalFrustum();

/**
 * @brief Whether the global frustum has been initialized
 */
bool IsGlobalFrustumAvailable();
