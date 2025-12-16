#include "Player.h"
#include "../physics/CollisionSystem.h"
#include "raylib.h"
#include "raymath.h"

void Player::UpdatePlayerMovement(Camera3D &camera)
{
    // Mouse look (only when cursor is hidden)
    if (IsCursorHidden())
    {
        Vector2 mouseDelta = GetMouseDelta();
        float sensitivity = 0.1f;

        cameraYaw -= mouseDelta.x * sensitivity;
        cameraPitch -= mouseDelta.y * sensitivity;

        // Clamp pitch to prevent flipping
        if (cameraPitch > 89.0f)
            cameraPitch = 89.0f;
        if (cameraPitch < -89.0f)
            cameraPitch = -89.0f;
    }
    // Calculate forward and sideways vectors based on camera rotation
    float yawRad = cameraYaw * DEG2RAD;
    Vector3 forward = {sinf(yawRad), 0.0f, cosf(yawRad)};
    Vector3 sideways = {cosf(yawRad), 0.0f, -sinf(yawRad)};

    // WASD movement relative to camera direction (FPS independent)
    float deltaTime = GetFrameTime();
    bool sprinting = IsKeyDown(KEY_LEFT_SHIFT);
    float speed = 5.0f; // Units per second (not per frame)
    float moveAmount = speed * deltaTime * (sprinting ? sprintMultiplier : 1.0f);

    Vector3 moveDirection = {0};

    if (IsKeyDown(KEY_W))
    {
        moveDirection = Vector3Add(moveDirection, forward);
        if (IsKeyDown(KEY_LEFT_SHIFT))
            moveAmount;
    }
    if (IsKeyDown(KEY_S))
    {
        moveDirection = Vector3Subtract(moveDirection, forward);
        if (IsKeyDown(KEY_LEFT_SHIFT))
            moveAmount;
    }
    if (IsKeyDown(KEY_A))
    {
        moveDirection = Vector3Add(moveDirection, sideways);
        if (IsKeyDown(KEY_LEFT_SHIFT))
            moveAmount;
    }
    if (IsKeyDown(KEY_D))
    {
        moveDirection = Vector3Subtract(moveDirection, sideways);
        if (IsKeyDown(KEY_LEFT_SHIFT))
            moveAmount;
    }

    // Normalize and apply movement
    if (Vector3Length(moveDirection) > 0.01f)
    {
        moveDirection = Vector3Normalize(moveDirection);
        position = Vector3Add(position, Vector3Scale(moveDirection, moveAmount));

        // Rotate player to face movement direction with smooth interpolation
        float targetYaw = atan2f(moveDirection.x, moveDirection.z) * RAD2DEG;

        // Calculate the shortest angle difference
        float angleDiff = targetYaw - playerYaw;
        while (angleDiff > 180.0f)
            angleDiff -= 360.0f;
        while (angleDiff < -180.0f)
            angleDiff += 360.0f;

        // Smoothly interpolate the yaw rotation
        float rotationSpeed = 10.0f; // Adjust this value for faster/slower rotation
        playerYaw += angleDiff * rotationSpeed * deltaTime;

        // Normalize angle to -180 to 180 range
        while (playerYaw > 180.0f)
            playerYaw -= 360.0f;
        while (playerYaw < -180.0f)
            playerYaw += 360.0f;
    }

    // Jump mechanics
    // Check if on ground
    isGrounded = (position.y <= groundLevel);

    // Jump only when on ground
    if (IsKeyDown(KEY_SPACE) && isGrounded)
    {
        velocityY = jumpStrength;
        isGrounded = false;
    }

    // Apply gravity
    velocityY += gravity * deltaTime;

    // Apply vertical velocity
    position.y += velocityY * deltaTime;

    // Ground collision
    if (position.y <= groundLevel)
    {
        position.y = groundLevel;
        velocityY = 0.0f;
        isGrounded = true;
    }

    // Position camera behind player with rotation
    float distance = 10.0f;
    float pitchRad = cameraPitch * DEG2RAD;

    camera.position.x = position.x - sinf(yawRad) * cosf(pitchRad) * distance;
    camera.position.y = position.y - sinf(pitchRad) * distance + 2.0f;
    camera.position.z = position.z - cosf(yawRad) * cosf(pitchRad) * distance;
    camera.target = Vector3Add(position, {0.0f, 1.0f, 0.0f});
}

void Player::PlayerRayCast()
{
    Ray ray = {0};
    ray.position = Vector3Add(position, {0.0f, eyeHeight, 0.0f}); // Eye level

    // Calculate forward direction based on player rotation
    float yawRad = playerYaw * DEG2RAD;
    ray.direction = {sinf(yawRad), 0.0f, cosf(yawRad)};

    // Draw ray for visualization
    Vector3 rayEnd = Vector3Add(ray.position, Vector3Scale(ray.direction, 10.0f));
    DrawLine3D(ray.position, rayEnd, BLUE);
}

void Player::UpdatePlayerMovementWithCollision(Camera3D &camera, CollisionSystem *collisionSystem)
{
    // Mouse look (only when cursor is hidden)
    if (IsCursorHidden())
    {
        Vector2 mouseDelta = GetMouseDelta();
        float sensitivity = 0.1f;

        cameraYaw -= mouseDelta.x * sensitivity;
        cameraPitch -= mouseDelta.y * sensitivity;

        // Clamp pitch to prevent flipping
        if (cameraPitch > 89.0f)
            cameraPitch = 89.0f;
        if (cameraPitch < -89.0f)
            cameraPitch = -89.0f;
    }

    // Calculate forward and sideways vectors based on camera rotation
    float yawRad = cameraYaw * DEG2RAD;
    Vector3 forward = {sinf(yawRad), 0.0f, cosf(yawRad)};
    Vector3 sideways = {cosf(yawRad), 0.0f, -sinf(yawRad)};

    // WASD movement relative to camera direction (FPS independent)
    float deltaTime = GetFrameTime();
    bool sprinting = IsKeyDown(KEY_LEFT_SHIFT);
    float speed = 5.0f; // Units per second (not per frame)
    float moveAmount = speed * deltaTime * (sprinting ? sprintMultiplier : 1.0f);

    Vector3 moveDirection = {0};

    if (IsKeyDown(KEY_W))
        moveDirection = Vector3Add(moveDirection, forward);
    if (IsKeyDown(KEY_S))
        moveDirection = Vector3Subtract(moveDirection, forward);
    if (IsKeyDown(KEY_A))
        moveDirection = Vector3Add(moveDirection, sideways);
    if (IsKeyDown(KEY_D))
        moveDirection = Vector3Subtract(moveDirection, sideways);

    // Normalize and apply movement with collision detection
    if (Vector3Length(moveDirection) > 0.01f)
    {
        moveDirection = Vector3Normalize(moveDirection);

        // Adjust movement direction to follow slope when on ground
        if (collisionSystem && isGrounded)
        {
            Vector3 groundNormal = collisionSystem->GetGroundNormal(position, collisionRadius);
            float slopeAngle = collisionSystem->GetSlopeAngle(position, collisionRadius);

            // If on a slope, project movement direction onto the slope surface
            if (slopeAngle > 0.1f) // Small threshold to avoid flat ground calculations
            {
                // Project movement onto slope plane using the ground normal
                // Formula: v_projected = v - (v · n) * n
                float dotProduct = Vector3DotProduct(moveDirection, groundNormal);
                Vector3 projectedDirection = Vector3Subtract(moveDirection, Vector3Scale(groundNormal, dotProduct));

                // Normalize the projected direction
                if (Vector3Length(projectedDirection) > 0.01f)
                {
                    moveDirection = Vector3Normalize(projectedDirection);
                }
            }
        }

        Vector3 attemptedPosition = Vector3Add(position, Vector3Scale(moveDirection, moveAmount));

        // Use collision system to resolve collisions
        if (collisionSystem)
        {
            position = collisionSystem->ResolvePlayerCollision(position, attemptedPosition, collisionRadius, collisionHeight);
        }
        else
        {
            position = attemptedPosition;
        }

        // Rotate player to face movement direction with smooth interpolation
        float targetYaw = atan2f(moveDirection.x, moveDirection.z) * RAD2DEG;

        // Calculate the shortest angle difference
        float angleDiff = targetYaw - playerYaw;
        while (angleDiff > 180.0f)
            angleDiff -= 360.0f;
        while (angleDiff < -180.0f)
            angleDiff += 360.0f;

        // Smoothly interpolate the yaw rotation
        float rotationSpeed = 10.0f;
        playerYaw += angleDiff * rotationSpeed * deltaTime;

        // Normalize angle to -180 to 180 range
        while (playerYaw > 180.0f)
            playerYaw -= 360.0f;
        while (playerYaw < -180.0f)
            playerYaw += 360.0f;
    }

    // Jump mechanics
    // Check ground height from collision system
    float detectedGroundLevel = groundLevel; // Default ground level
    bool onWalkableGround = true;

    if (collisionSystem)
    {
        float groundBelow = collisionSystem->GetGroundHeightBelow(position, collisionRadius, 5.0f);
        detectedGroundLevel = fmaxf(groundLevel, groundBelow);

        // Check if the ground is walkable (slope < 45 degrees)
        onWalkableGround = collisionSystem->IsGroundWalkable(position, collisionRadius, 45.0f);
    }

    isGrounded = (position.y <= detectedGroundLevel + 0.01f);

    if (IsKeyDown(KEY_SPACE) && isGrounded && onWalkableGround)
    {
        velocityY = jumpStrength;
        isGrounded = false;
    }

    // Apply gravity only when not grounded (in the air)
    if (!isGrounded)
    {
        velocityY += gravity * deltaTime;
    }

    // Apply vertical velocity
    position.y += velocityY * deltaTime;

    // Ground collision
    if (position.y <= detectedGroundLevel)
    {
        position.y = detectedGroundLevel;
        velocityY = 0.0f;
        isGrounded = true;

        // TODO: If on steep slope (> 45 degrees) and moving down, apply sliding force
        // This will make the player slide down steep slopes automatically
    }

    // Position camera behind player with rotation
    float distance = 10.0f;
    float pitchRad = cameraPitch * DEG2RAD;

    camera.position.x = position.x - sinf(yawRad) * cosf(pitchRad) * distance;
    camera.position.y = position.y - sinf(pitchRad) * distance + 2.0f;
    camera.position.z = position.z - cosf(yawRad) * cosf(pitchRad) * distance;
    camera.target = Vector3Add(position, {0.0f, 1.0f, 0.0f});
}