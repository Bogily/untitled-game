#include "Player.h"
#include "raymath.h"
#include "rlgl.h"
#include "../physics/SDFCollisionSystem.h"

void Player::Update()
{
    cameraPitch = Clamp(cameraPitch, -89.0f, 89.0f);
}

void Player::UpdateMovement(const Camera3D &camera,
                            float deltaTime,
                            float moveSpeed,
                            float gravity,
                            float collisionRadius,
                            float &verticalVelocity,
                            bool collisionEnabled,
                            const SDFCollisionSystem *collisionSystem)
{
    Vector3 &pos = transform.position;

    Vector3 camForward = Vector3Subtract(camera.target, camera.position);
    camForward.y = 0.0f;

    float forwardLen = Vector3Length(camForward);
    if (forwardLen > 1e-6f)
        camForward = Vector3Scale(camForward, 1.0f / forwardLen);
    else
        camForward = {0.0f, 0.0f, -1.0f};

    Vector3 camRight = Vector3CrossProduct(camForward, {0.0f, 1.0f, 0.0f});
    camRight = Vector3Normalize(camRight);

    Vector3 moveDir = {0.0f, 0.0f, 0.0f};
    if (IsKeyDown(KEY_W))
        moveDir = Vector3Add(moveDir, camForward);
    if (IsKeyDown(KEY_S))
        moveDir = Vector3Subtract(moveDir, camForward);
    if (IsKeyDown(KEY_D))
        moveDir = Vector3Add(moveDir, camRight);
    if (IsKeyDown(KEY_A))
        moveDir = Vector3Subtract(moveDir, camRight);

    float moveDirLen = Vector3Length(moveDir);
    if (moveDirLen > 1e-6f)
    {
        moveDir = Vector3Scale(moveDir, 1.0f / moveDirLen);

        float speed = moveSpeed;
        if (IsKeyDown(KEY_LEFT_SHIFT))
            speed *= sprintMultiplier;

        pos.x += moveDir.x * speed * deltaTime;
        pos.z += moveDir.z * speed * deltaTime;

        playerYaw = atan2f(moveDir.x, moveDir.z) * RAD2DEG;
    }

    verticalVelocity -= gravity * deltaTime;
    pos.y += verticalVelocity * deltaTime;

    if (collisionEnabled && collisionSystem && collisionSystem->IsReady())
    {
        Vector3 resolved = collisionSystem->ResolvePosition(pos, collisionRadius, 4);

        if (resolved.y > pos.y + 1e-4f && verticalVelocity < 0.0f)
            verticalVelocity = 0.0f;

        pos = resolved;
    }
}

void Player::Draw()
{
    if (!render.modelLoaded)
        return;

    rlPushMatrix();
    rlTranslatef(transform.position.x, transform.position.y, transform.position.z);
    rlRotatef(render.modelRotationOffset.y + playerYaw, 0.0f, 1.0f, 0.0f);
    rlRotatef(render.modelRotationOffset.x, 1.0f, 0.0f, 0.0f);
    rlRotatef(render.modelRotationOffset.z, 0.0f, 0.0f, 1.0f);
    rlScalef(render.modelScale.x, render.modelScale.y, render.modelScale.z);
    DrawModel(render.model, {0.0f, 0.0f, 0.0f}, 1.0f, render.tint);
    rlPopMatrix();
}

Ray Player::GetForwardRay() const
{
    const float yawRad = cameraYaw * DEG2RAD;
    const float pitchRad = cameraPitch * DEG2RAD;

    Vector3 direction = {
        sinf(yawRad) * cosf(pitchRad),
        sinf(pitchRad),
        cosf(yawRad) * cosf(pitchRad)};

    Ray ray = {};
    ray.position = Vector3Add(transform.position, {0.0f, eyeHeight, 0.0f});
    ray.direction = Vector3Normalize(direction);
    return ray;
}
