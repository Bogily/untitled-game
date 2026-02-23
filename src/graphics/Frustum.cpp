#include "Frustum.h"
#include "raymath.h"

namespace
{
    Frustum gGlobalFrustum{};
    bool gHasGlobalFrustum = false;
}

Frustum BuildFrustumFromCamera(const Camera3D &camera)
{
    Frustum frustum;

    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    Matrix viewProj = MatrixMultiply(GetCameraMatrix(camera),
                                     MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.1f, 1000.0f));

    // Left
    frustum.planes[0].normal = {viewProj.m3 + viewProj.m0, viewProj.m7 + viewProj.m4, viewProj.m11 + viewProj.m8};
    frustum.planes[0].distance = viewProj.m15 + viewProj.m12;
    // Right
    frustum.planes[1].normal = {viewProj.m3 - viewProj.m0, viewProj.m7 - viewProj.m4, viewProj.m11 - viewProj.m8};
    frustum.planes[1].distance = viewProj.m15 - viewProj.m12;
    // Bottom
    frustum.planes[2].normal = {viewProj.m3 + viewProj.m1, viewProj.m7 + viewProj.m5, viewProj.m11 + viewProj.m9};
    frustum.planes[2].distance = viewProj.m15 + viewProj.m13;
    // Top
    frustum.planes[3].normal = {viewProj.m3 - viewProj.m1, viewProj.m7 - viewProj.m5, viewProj.m11 - viewProj.m9};
    frustum.planes[3].distance = viewProj.m15 - viewProj.m13;
    // Near
    frustum.planes[4].normal = {viewProj.m3 + viewProj.m2, viewProj.m7 + viewProj.m6, viewProj.m11 + viewProj.m10};
    frustum.planes[4].distance = viewProj.m15 + viewProj.m14;
    // Far
    frustum.planes[5].normal = {viewProj.m3 - viewProj.m2, viewProj.m7 - viewProj.m6, viewProj.m11 - viewProj.m10};
    frustum.planes[5].distance = viewProj.m15 - viewProj.m14;

    for (int i = 0; i < 6; i++)
    {
        float length = Vector3Length(frustum.planes[i].normal);
        if (length > 0.0f)
        {
            frustum.planes[i].normal = Vector3Scale(frustum.planes[i].normal, 1.0f / length);
            frustum.planes[i].distance /= length;
        }
    }

    frustum.planes[4].distance -= 5.0f;

    return frustum;
}

void SetGlobalFrustum(const Frustum &frustum)
{
    gGlobalFrustum = frustum;
    gHasGlobalFrustum = true;
}

void UpdateGlobalFrustum(const Camera3D &camera)
{
    SetGlobalFrustum(BuildFrustumFromCamera(camera));
}

const Frustum &GetGlobalFrustum()
{
    return gGlobalFrustum;
}

bool IsGlobalFrustumAvailable()
{
    return gHasGlobalFrustum;
}
