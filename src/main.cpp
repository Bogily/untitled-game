#include "raylib.h"
#include "raymath.h"
#include "player/Player.h"
#include "utils/custommodel.h"
#include "utils/DebugMenu.h"

// Simple constant for screen dimensions
const int SCREEN_WIDTH = GetScreenWidth();
const int SCREEN_HEIGHT = GetScreenHeight();

int main()
{
    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Zelda-like 3D Game Structure skid");

    // Define the camera to look into our 3d world
    Camera3D camera = {0};
    camera.position = {0.0f, 10.0f, 10.0f}; // Camera position
    camera.target = {0.0f, 0.0f, 0.0f};     // Camera looking at point
    camera.up = {0.0f, 1.0f, 0.0f};         // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                             // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;          // Camera projection type

    // Create player instance
    Player player;
    player.position = {0.0f, 1.0f, 0.0f};
    
    // Load rat model with texture
    CustomModel customModel;
    customModel.loadPlayerModel(player, "assets/models/rat.obj", "assets/textures/rat.png");

    SetTargetFPS(165);
    SetExitKey(KEY_NULL); // Disable ESC to close window
    
    // Lock mouse cursor
    DisableCursor();
    
    // Debug Menu Setup
    DebugMenu debugMenu;
    bool showGrid = true;
    bool showRaycast = true;
    bool showPlayerPos = false;
    bool showFPS = true;
    
    // Add debug settings (you can add more here!)
    debugMenu.AddBool("Show Grid", &showGrid);
    debugMenu.AddBool("Show Raycast", &showRaycast);
    debugMenu.AddBool("Show Player Position", &showPlayerPos);
    debugMenu.AddBool("Show FPS", &showFPS);
    debugMenu.AddFloat("Jump Strength", &player.jumpStrength, 1.0f, 20.0f, 0.1f);
    debugMenu.AddFloat("Gravity", &player.gravity, -50.0f, -5.0f, 0.1f);
    debugMenu.AddFloat("Camera Distance", &camera.fovy, 20.0f, 90.0f, 0.1f);
    debugMenu.AddFloat("Sprint Multiplier", &player.sprintMultiplier, 1.0f, 5.0f, 0.1f);

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update
        // ----------------------------------------------------------------------------------
        // Update debug menu
        debugMenu.Update();
        
        // Toggle cursor lock with TAB key
        if (IsKeyPressed(KEY_TAB))
        {
            if (IsCursorHidden())
                EnableCursor();
            else
                DisableCursor();
        }
        
        // Close window with ESC
        if (IsKeyPressed(KEY_DELETE))
            break;
        
        // Simple movement logic for the "Player" -> view src/player/movement.cpp
        player.UpdatePlayerMovement(camera);
        
        // ----------------------------------------------------------------------------------

        // Draw
        // ----------------------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        
        // Draw raycast (if enabled)
        if (showRaycast)
            player.PlayerRayCast();
        
        // Draw ground
        DrawPlane({0.0f, 0.0f, 0.0f}, {32.0f, 32.0f}, LIGHTGRAY);
        
        // Draw grid (if enabled)
        if (showGrid)
            DrawGrid(20, 1.0f);

        // Draw player model
        customModel.drawPlayerModel(player);
        
        // Draw environment objects
        DrawCube({-4.0f, 1.0f, -4.0f}, 2.0f, 2.0f, 2.0f, RED);
        DrawCube({4.0f, 1.0f, 4.0f}, 1.0f, 4.0f, 1.0f, BLUE);

        EndMode3D();

        // UI
        DrawText("WASD: Move | Mouse: Look | TAB: Toggle Cursor | ESC: Exit", 10, 10, 20, DARKGRAY);
        
        // Show FPS (if enabled)
        if (showFPS)
            DrawFPS(10, 40);
        
        // Show player position (if enabled)
        if (showPlayerPos)
        {
            char posText[128];
            snprintf(posText, sizeof(posText), "Position: X:%.1f Y:%.1f Z:%.1f", 
                     player.position.x, player.position.y, player.position.z);
            DrawText(posText, 10, 70, 20, DARKGREEN);
        }
        
        // Draw debug menu
        debugMenu.Draw();

        EndDrawing();
        // ----------------------------------------------------------------------------------
    }

    // De-Initialization
    if (player.modelLoaded)
        UnloadModel(player.model);
    
    CloseWindow();
    return 0;
}
