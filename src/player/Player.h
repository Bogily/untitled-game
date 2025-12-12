#pragma once
#include "raylib.h"

class Player
{
public:
    Vector3 position;
    float playerYaw = 0.0f;   // Player's body rotation
    float cameraYaw = 0.0f;   // Horizontal rotation
    float cameraPitch = -30.0f; // Vertical rotation
    Model model;
    bool modelLoaded = false;
    float sprintMultiplier = 2.0f;
    
    // Jump mechanics
    float velocityY = 0.0f;       // Vertical velocity
    float gravity = -20.0f;       // Gravity acceleration
    float jumpStrength = 8.0f;    // Initial jump velocity
    bool isGrounded = true;       // Whether player is on ground
    float groundLevel = 1.0f;     // Ground height
    
    void Update();
    void Draw();
    void UpdatePlayerMovement(Camera3D& camera);
    void PlayerRayCast();
};
