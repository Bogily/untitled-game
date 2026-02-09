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
