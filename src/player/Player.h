#pragma once
#include "raylib.h"
#include <vector>

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
    float eyeHeight = 1.6f;       // Eye/camera offset from feet for raycasting

    void Update();
    void Draw();
    Ray GetForwardRay() const;
};
