#pragma once
#include "raylib.h"

// Frustum plane structure for culling
struct FrustumPlane
{
    Vector3 normal;
    float distance;
};

struct Frustum
{
    FrustumPlane planes[6]; // Near, Far, Left, Right, Top, Bottom
};
