#include "raylib.h"
#include "raymath.h"
#include "player/Player.h"
#include "utils/custommodel.h"
#include "utils/DebugMenu.h"
#include "graphics/Skybox.h"
#include "graphics/BillboardText.h"
#include "graphics/CameraController.h"

// Simple constant for screen dimensions
const int SCREEN_WIDTH = GetScreenWidth();
const int SCREEN_HEIGHT = GetScreenHeight();
const int TARGET_FPS = 60;

int main()
{
    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Zelda-like 3D Game Structure skid");

    // Initialize camera controller
    CameraController cameraController;
    cameraController.Initialize(
        (Vector3){0.0f, 10.0f, 10.0f}, // Camera position
        (Vector3){0.0f, 0.0f, 0.0f},   // Looking at
        45.0f                          // FOV
    );

    // Create player instance
    Player player;
    player.position = {0.0f, 1.0f, 0.0f};

    // Set camera to follow player initially
    cameraController.SetMode(CAMERA_MODE_FOLLOW);
    cameraController.SetFollowTarget(&player.position);
    cameraController.SetFollowDistance(10.0f);
    cameraController.SetFollowHeight(5.0f);
    cameraController.SetSmoothness(0.15f);

    // Load rat model with texture
    CustomModel customModel;
    customModel.loadPlayerModel(player, "assets/models/rat.obj", "assets/textures/rat.png");

    // Load skybox
    Skybox skybox;
    skybox.Load("assets/shader/skybox.vs", "assets/shader/skybox.fs");
    // Customize sky and cloud colors
    skybox.SetSkyColor({0.3f, 0.5f, 0.9f});   // Blue sky (default)
    skybox.SetCloudColor({1.0f, 1.0f, 1.0f}); // White clouds (default)

    SetTargetFPS(TARGET_FPS);
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
        
        float deltaTime = GetFrameTime();

        // Update camera controller
        cameraController.Update(deltaTime);

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
        if (IsKeyPressed(KEY_DELETE))
            break;
        
        // Simple movement logic for the "Player" -> view src/player/movement.cpp
        player.UpdatePlayerMovement(camera);
        

        // Camera mode examples with number keys
        // 1 - Follow player (default)
        if (IsKeyPressed(KEY_ONE))
        {
            cameraController.SetMode(CAMERA_MODE_FOLLOW);
            cameraController.SetFollowTarget(&player.position);
        }

        // 2 - Cinematic overview cutscene
        if (IsKeyPressed(KEY_TWO))
        {
            std::vector<CameraWaypoint> overviewCutscene;
            overviewCutscene.push_back({(Vector3){0.0f, 30.0f, 30.0f}, (Vector3){0.0f, 0.0f, 0.0f}, 3.0f, 60.0f});
            overviewCutscene.push_back({(Vector3){20.0f, 25.0f, 0.0f}, (Vector3){0.0f, 0.0f, 0.0f}, 3.0f, 50.0f});
            overviewCutscene.push_back({(Vector3){0.0f, 25.0f, -20.0f}, (Vector3){0.0f, 0.0f, 0.0f}, 3.0f, 50.0f});
            overviewCutscene.push_back({(Vector3){-20.0f, 20.0f, 0.0f}, (Vector3){0.0f, 0.0f, 0.0f}, 3.0f, 55.0f});
            cameraController.StartCutscene(overviewCutscene);
        }

        // 3 - Dramatic zoom cutscene
        if (IsKeyPressed(KEY_THREE))
        {
            std::vector<CameraWaypoint> zoomCutscene;
            zoomCutscene.push_back({(Vector3){player.position.x, player.position.y + 15.0f, player.position.z + 15.0f},
                                    player.position, 2.0f, 70.0f});
            zoomCutscene.push_back({(Vector3){player.position.x, player.position.y + 5.0f, player.position.z + 5.0f},
                                    player.position, 2.0f, 35.0f});
            zoomCutscene.push_back({(Vector3){player.position.x + 3.0f, player.position.y + 2.0f, player.position.z + 3.0f},
                                    player.position, 1.5f, 45.0f});
            cameraController.StartCutscene(zoomCutscene);
        }

        // 4 - Fixed camera looking at red cube
        if (IsKeyPressed(KEY_FOUR))
        {
            cameraController.TransitionTo(
                (Vector3){-10.0f, 8.0f, 0.0f},
                (Vector3){-4.0f, 1.0f, -4.0f},
                1.5f);
            cameraController.SetMode(CAMERA_MODE_FIXED);
        }

        // 5 - Orbit cutscene around blue tower
        if (IsKeyPressed(KEY_FIVE))
        {
            std::vector<CameraWaypoint> orbitCutscene;
            Vector3 towerPos = {4.0f, 3.0f, 4.0f};
            float radius = 8.0f;
            for (int i = 0; i < 8; i++)
            {
                float angle = (i / 8.0f) * 2.0f * PI;
                Vector3 camPos = {
                    towerPos.x + radius * cosf(angle),
                    towerPos.y + 3.0f,
                    towerPos.z + radius * sinf(angle)};
                orbitCutscene.push_back({camPos, towerPos, 1.5f, 45.0f});
            }
            cameraController.StartCutscene(orbitCutscene);
        }

        // 6 - Bird's eye view
        if (IsKeyPressed(KEY_SIX))
        {
            cameraController.TransitionTo(
                (Vector3){0.0f, 40.0f, 0.1f},
                (Vector3){0.0f, 0.0f, 0.0f},
                2.0f);
            cameraController.SetMode(CAMERA_MODE_FIXED);
        }

        // Only update player movement if not in cutscene or fixed camera
        if (!cameraController.IsCutscenePlaying() &&
            cameraController.mode != CAMERA_MODE_FIXED)
        {
            player.UpdatePlayerMovement(cameraController.camera);
        }
        else if (cameraController.mode == CAMERA_MODE_FIXED)
        {
            // In fixed camera mode, still allow player movement but don't update camera
            // Just handle basic WASD movement without camera control
            float deltaTime = GetFrameTime();
            float speed = 5.0f;
            float moveAmount = speed * deltaTime;

            Vector3 moveDirection = {0};
            if (IsKeyDown(KEY_W))
                moveDirection.z += 1.0f;
            if (IsKeyDown(KEY_S))
                moveDirection.z -= 1.0f;
            if (IsKeyDown(KEY_A))
                moveDirection.x -= 1.0f;
            if (IsKeyDown(KEY_D))
                moveDirection.x += 1.0f;

            if (Vector3Length(moveDirection) > 0.01f)
            {
                moveDirection = Vector3Normalize(moveDirection);
                player.position = Vector3Add(player.position, Vector3Scale(moveDirection, moveAmount));
            }

            if (IsKeyDown(KEY_SPACE) && player.position.y < 5.0f)
                player.position.y += moveAmount;
            if (IsKeyDown(KEY_LEFT_SHIFT))
                player.position.y -= moveAmount;
            if (player.position.y < 1.0f)
                player.position.y = 1.0f;
        }

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

        BeginMode3D(cameraController.camera);

        // Draw skybox first (behind everything)
        skybox.Draw(cameraController.camera);

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

        // Billboard text rendering (after EndMode3D, before EndDrawing)
        // Example 1: Simple billboard text above red cube
        BillboardText::DrawText3D("Red Cube", (Vector3){-4.0f, 3.0f, -4.0f}, cameraController.camera, 20, RED);

        // Example 2: Billboard text with background above blue cube
        BillboardText::DrawText3DWithBackground("Blue Tower", (Vector3){4.0f, 5.5f, 4.0f}, cameraController.camera, 20, WHITE, Fade(BLUE, 0.7f));

        // Example 3: Distance-scaled text above player
        BillboardText::DrawText3DScaled("Player", Vector3Add(player.position, (Vector3){0.0f, 2.0f, 0.0f}), cameraController.camera, 40.0f, 20.0f, GREEN);

        // Example 4: Text with connecting line
        BillboardText::DrawText3DWithLine("Target", (Vector3){0.0f, 0.0f, 0.0f}, cameraController.camera, 18, YELLOW, ORANGE, 40.0f);

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

        // Camera mode display
        const char *modeText = "";
        switch (cameraController.mode)
        {
        case CAMERA_MODE_FREE:
            modeText = "FREE";
            break;
        case CAMERA_MODE_FOLLOW:
            modeText = "FOLLOW";
            break;
        case CAMERA_MODE_CUTSCENE:
            modeText = "CUTSCENE";
            break;
        case CAMERA_MODE_FIXED:
            modeText = "FIXED";
            break;
        }
        DrawText(TextFormat("Camera Mode: %s", modeText), 10, 40, 20, GREEN);

        if (cameraController.IsCutscenePlaying())
        {
            DrawText("[CUTSCENE PLAYING]", 10, 70, 20, RED);
        }

        DrawText("Camera Controls:", 10, 100, 16, DARKGRAY);
        DrawText("1: Follow Player | 2: Overview | 3: Zoom | 4: Red Cube | 5: Orbit Tower | 6: Bird's Eye", 10, 120, 16, DARKGRAY);

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
