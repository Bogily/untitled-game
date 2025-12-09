#include "raylib.h"
#include "raymath.h"

// Simple constant for screen dimensions
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

int main()
{
    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Zelda-like 3D Game Structure");

    // Define the camera to look into our 3d world
    Camera3D camera = {0};
    camera.position = (Vector3){0.0f, 10.0f, 10.0f}; // Camera position
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};     // Camera looking at point
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};         // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                             // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;          // Camera projection type

    // Player placeholder variables
    Vector3 playerPosition = {0.0f, 1.0f, 0.0f};
    Color playerColor = GREEN;

    SetTargetFPS(60); // Set our game to run at 60 frames-per-second

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        // Update
        // ----------------------------------------------------------------------------------
        // Simple movement logic for the "Player"
        if (IsKeyDown(KEY_W))
            playerPosition.z -= 0.1f;
        if (IsKeyDown(KEY_S))
            playerPosition.z += 0.1f;
        if (IsKeyDown(KEY_A))
            playerPosition.x -= 0.1f;
        if (IsKeyDown(KEY_D))
            playerPosition.x += 0.1f;

        // Camera follows player (Third Person-ish)
        camera.target = playerPosition;
        camera.position.x = playerPosition.x;
        camera.position.z = playerPosition.z + 10.0f;
        camera.position.y = 10.0f;
        // ----------------------------------------------------------------------------------

        // Draw
        // ----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        // Draw ground
        DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){32.0f, 32.0f}, LIGHTGRAY);
        DrawGrid(20, 1.0f);

        // Draw "Player" (Link placeholder)
        DrawCube(playerPosition, 1.0f, 2.0f, 1.0f, playerColor);
        DrawCubeWires(playerPosition, 1.0f, 2.0f, 1.0f, DARKGREEN);

        // Draw some environment objects
        DrawCube((Vector3){-4.0f, 1.0f, -4.0f}, 2.0f, 2.0f, 2.0f, RED);
        DrawCube((Vector3){4.0f, 1.0f, 4.0f}, 1.0f, 4.0f, 1.0f, BLUE);

        EndMode3D();

        DrawText("WASD to move the character", 10, 10, 20, DARKGRAY);
        DrawText("Zelda-like Structure Demo", 10, 40, 10, GRAY);

        EndDrawing();
        // ----------------------------------------------------------------------------------
    }

    // De-Initialization
    CloseWindow(); // Close window and OpenGL context

    return 0;
}
