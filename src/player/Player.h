#pragma once
#include "raylib.h"
#include <vector>

// Forward declaration
class CollisionSystem;

class Player
{
public:
    Vector3 position;
    float playerYaw = 0.0f;     // Player's body rotation
    float cameraYaw = 0.0f;     // Horizontal rotation
    float cameraPitch = -30.0f; // Vertical rotation
    Model model;
    bool modelLoaded = false;
    Vector3 modelScale = {1.0f, 1.0f, 1.0f};
    Vector3 modelRotationOffset = {0.0f, 0.0f, 0.0f}; // X, Y, Z rotation offsets
    float sprintMultiplier = 2.0f;

    // Jump mechanics
    float velocityY = 0.0f;      // Vertical velocity
    float gravity = -20.0f;      // Gravity acceleration
    float jumpStrength = 8.0f;   // Initial jump velocity
    bool isGrounded = true;      // Whether player is on ground
    float groundLevel = -900.0f; // Ground height

    // Collision (separate from visual representation)
    float collisionRadius = 0.5f; // Radius for cylindrical collision
    float collisionHeight = 2.0f; // Height of collision cylinder (body height)
    float eyeHeight = 1.6f;       // Eye/camera offset from feet for raycasting

    void Update();
    void Draw();
    void UpdatePlayerMovement(Camera3D &camera);
    void UpdatePlayerMovementWithCollision(Camera3D &camera, CollisionSystem *collisionSystem);
    void PlayerRayCast();
    Ray GetForwardRay() const;
};
