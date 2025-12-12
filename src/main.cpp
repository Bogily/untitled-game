#include "raylib.h"
#include "raymath.h"
#include "player/Player.h"
#include "utils/custommodel.h"
#include "graphics/Skybox.h"

// Simple constant for screen dimensions
const int SCREEN_WIDTH = GetScreenWidth();
const int SCREEN_HEIGHT = GetScreenHeight();

int main()
{
    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Zelda-like 3D Game Structure skid");

    // Define the camera to look into our 3d world
    Camera3D camera = {0};
    camera.position = (Vector3){0.0f, 10.0f, 10.0f}; // Camera position
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};     // Camera looking at point
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};         // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                             // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;          // Camera projection type

    // Create player instance
    Player player;
    player.position = {0.0f, 1.0f, 0.0f};

    // Load rat model with texture
    CustomModel customModel;
    customModel.loadPlayerModel(player, "assets/models/rat.obj", "assets/textures/rat.png");

    // Load skybox
    Skybox skybox;
    skybox.Load("assets/shader/skybox.vs", "assets/shader/skybox.fs");
    // Customize sky and cloud colors
    skybox.SetSkyColor({0.3f, 0.5f, 0.9f});   // Blue sky (default)
    skybox.SetCloudColor({1.0f, 1.0f, 1.0f}); // White clouds (default)

    SetTargetFPS(60);
    SetExitKey(KEY_NULL); // Disable ESC to close window

    // Lock mouse cursor
    DisableCursor();

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update
        // ----------------------------------------------------------------------------------
        float deltaTime = GetFrameTime();

        // Update skybox animation
        skybox.Update(deltaTime);

        // Toggle cursor lock with TAB key
        if (IsKeyPressed(KEY_TAB))
        {
            if (IsCursorHidden())
                EnableCursor();
            else
                DisableCursor();
        }

        // Close window with ESC
        if (IsKeyPressed(KEY_ESCAPE))
            break;

        // Simple movement logic for the "Player" -> view src/player/movement.cpp
        player.UpdatePlayerMovement(camera);

        // Check collision with a cube (example)
        /*BoundingBox cubeBox = {
            (Vector3){-4.0f - 1.0f, 0.0f, -4.0f - 1.0f},
            (Vector3){-4.0f + 1.0f, 2.0f, -4.0f + 1.0f}
        };
        RayCollision collision = GetRayCollisionBox(ray, cubeBox);*/

        // ----------------------------------------------------------------------------------

        // Draw
        // ----------------------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        // Draw skybox first (behind everything)
        skybox.Draw(camera);

        player.PlayerRayCast();
        // Draw ground
        DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){32.0f, 32.0f}, LIGHTGRAY);
        DrawGrid(20, 1.0f);

        // Draw player model
        customModel.drawPlayerModel(player);

        // Draw environment objects
        DrawCube((Vector3){-4.0f, 1.0f, -4.0f}, 2.0f, 2.0f, 2.0f, RED);
        DrawCube((Vector3){4.0f, 1.0f, 4.0f}, 1.0f, 4.0f, 1.0f, BLUE);

        EndMode3D();

        // UI
        DrawText("WASD: Move | Mouse: Look | TAB: Toggle Cursor | ESC: Exit", 10, 10, 20, DARKGRAY);

        EndDrawing();
        // ----------------------------------------------------------------------------------
    }

    // De-Initialization
    skybox.Unload();

    if (player.modelLoaded)
        UnloadModel(player.model);

    CloseWindow();
    return 0;
}
