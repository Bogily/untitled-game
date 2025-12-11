#include "Player.h"
#include "raylib.h"
#include "raymath.h"

void Player::UpdatePlayerMovement(Camera3D& camera)
{
    // Mouse look (only when cursor is hidden)
    if (IsCursorHidden())
    {
        Vector2 mouseDelta = GetMouseDelta();
        float sensitivity = 0.1f;
        
        cameraYaw -= mouseDelta.x * sensitivity;
        cameraPitch -= mouseDelta.y * sensitivity;
        
        // Clamp pitch to prevent flipping
        if (cameraPitch > 89.0f) cameraPitch = 89.0f;
        if (cameraPitch < -89.0f) cameraPitch = -89.0f;
    }
    // Calculate forward and right vectors based on camera rotation
    float yawRad = cameraYaw * DEG2RAD;
    Vector3 forward = {sinf(yawRad), 0.0f, cosf(yawRad)};
    Vector3 right = {cosf(yawRad), 0.0f, -sinf(yawRad)};
    
    // WASD movement relative to camera direction (FPS independent)
    float deltaTime = GetFrameTime();
    float speed = 5.0f; // Units per second (not per frame)
    float moveAmount = speed * deltaTime;
    
    Vector3 moveDirection = {0};
    
    if (IsKeyDown(KEY_W))
        moveDirection = Vector3Add(moveDirection, forward);
    if (IsKeyDown(KEY_S))
        moveDirection = Vector3Subtract(moveDirection, forward);
    if (IsKeyDown(KEY_A))
        moveDirection = Vector3Add(moveDirection, right);
    if (IsKeyDown(KEY_D))
        moveDirection = Vector3Subtract(moveDirection, right);
    
    // Normalize and apply movement
    if (Vector3Length(moveDirection) > 0.01f)
    {
        moveDirection = Vector3Normalize(moveDirection);
        position = Vector3Add(position, Vector3Scale(moveDirection, moveAmount));
        
        // Rotate player to face movement direction
        playerYaw = atan2f(moveDirection.x, moveDirection.z) * RAD2DEG;
    }
    
    if (IsKeyDown(KEY_SPACE))
        position.y += moveAmount;
    if (IsKeyDown(KEY_LEFT_SHIFT))
        position.y -= moveAmount;
    if (position.y < 1.0f)
        position.y = 1.0f; // Prevent going below ground
    
    // Position camera behind player with rotation
    float distance = 10.0f;
    float pitchRad = cameraPitch * DEG2RAD;
    
    camera.position.x = position.x - sinf(yawRad) * cosf(pitchRad) * distance;
    camera.position.y = position.y - sinf(pitchRad) * distance + 2.0f;
    camera.position.z = position.z - cosf(yawRad) * cosf(pitchRad) * distance;
    camera.target = Vector3Add(position, (Vector3){0.0f, 1.0f, 0.0f});
}

void Player::PlayerRayCast()
{
    Ray ray = {0};
    ray.position = Vector3Add(position, (Vector3){0.0f, 1.0f, 0.0f}); // Eye level
    
    // Calculate forward direction based on player rotation
    float yawRad = playerYaw * DEG2RAD;
    ray.direction = (Vector3){sinf(yawRad), 0.0f, cosf(yawRad)};

    // Draw ray for visualization
    Vector3 rayEnd = Vector3Add(ray.position, Vector3Scale(ray.direction, 10.0f));
    DrawLine3D(ray.position, rayEnd, BLUE);
}