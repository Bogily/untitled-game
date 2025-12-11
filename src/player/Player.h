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
    void Update();
    void Draw();
    void UpdatePlayerMovement(Camera3D& camera);
    void PlayerRayCast();
};
