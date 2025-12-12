#include "Game.h"

// Constants
namespace GameConstants
{
    constexpr Vector3 WORLD_CENTER = {0.0f, 0.0f, 0.0f};
    constexpr Vector3 PLAYER_START_POS = {0.0f, 1.0f, 0.0f};
    constexpr Vector3 CAMERA_START_POS = {0.0f, 10.0f, 10.0f};
    constexpr float CAMERA_START_FOV = 45.0f;
    constexpr float CAMERA_FOLLOW_DISTANCE = 10.0f;
    constexpr float CAMERA_FOLLOW_HEIGHT = 5.0f;
    constexpr float CAMERA_SMOOTHNESS = 0.15f;
    
    // World objects
    constexpr Vector3 RED_CUBE_POS = {-4.0f, 1.0f, -4.0f};
    constexpr Vector3 BLUE_TOWER_POS = {4.0f, 1.0f, 4.0f};
    constexpr Vector3 PLANE_SIZE = {32.0f, 32.0f, 0.0f};
    constexpr int GRID_SIZE = 20;
}

void Game::Init()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Zelda-like 3D Game Structure skid");
    SetTargetFPS(TARGET_FPS);
    SetExitKey(KEY_NULL);
    DisableCursor();
    
    SetupCamera();
    SetupPlayer();
    SetupModels();
    SetupSkybox();
    SetupDebugMenu();
}

void Game::SetupCamera()
{
    cameraController.Initialize(
        GameConstants::CAMERA_START_POS,
        GameConstants::WORLD_CENTER,
        GameConstants::CAMERA_START_FOV
    );
    
    cameraController.SetMode(CAMERA_MODE_FOLLOW);
    cameraController.SetFollowTarget(&player.position);
    cameraController.SetFollowDistance(GameConstants::CAMERA_FOLLOW_DISTANCE);
    cameraController.SetFollowHeight(GameConstants::CAMERA_FOLLOW_HEIGHT);
    cameraController.SetSmoothness(GameConstants::CAMERA_SMOOTHNESS);
}

void Game::SetupPlayer()
{
    player.position = GameConstants::PLAYER_START_POS;
}

void Game::SetupModels()
{
    customModel.addModel("Rat", "assets/models/rat.obj", "assets/textures/rat.png", {0.04f, 0.04f, 0.04f}, {0.0f, 0.0f, 0.0f});
    customModel.addModel("Miku", "assets/models/miku/scene.gltf", "", {1.8f, 1.8f, 1.8f}, {90.0f, 0.0f, 0.0f});
    customModel.loadPlayerModel(player, currentModelIndex);
}

void Game::SetupSkybox()
{
    skybox.Load("assets/shader/skybox.vs", "assets/shader/skybox.fs");
    skybox.SetSkyColor({0.0f, 0.0f, 0.0f});
    skybox.SetCloudColor({1.0f, 1.0f, 0.0f});
}

void Game::SetupDebugMenu()
{
    debugMenu.AddBool("Show Grid", &showGrid);
    debugMenu.AddBool("Show Raycast", &showRaycast);
    debugMenu.AddBool("Show Player Position", &showPlayerPos);
    debugMenu.AddBool("Show FPS", &showFPS);
    debugMenu.AddFloat("Jump Strength", &player.jumpStrength, 1.0f, 20.0f, 0.1f);
    debugMenu.AddFloat("Gravity", &player.gravity, -50.0f, -5.0f, 0.1f);
    debugMenu.AddFloat("Sprint Multiplier", &player.sprintMultiplier, 1.0f, 5.0f, 0.1f);
    
    std::vector<std::string> modelNames;
    for (int i = 0; i < customModel.getModelCount(); i++)
        modelNames.push_back(customModel.getModelName(i));
    debugMenu.AddString("Player Model", &currentModelIndex, modelNames);
    
    debugMenu.AddFloat("Camera FOV", &cameraController.camera.fovy, 20.0f, 120.0f, 1.0f);
}

void Game::Update()
{
    float deltaTime = GetFrameTime();
    
    debugMenu.Update();
    
    if (currentModelIndex != previousModelIndex)
    {
        customModel.loadPlayerModel(player, currentModelIndex);
        previousModelIndex = currentModelIndex;
    }
    
    cameraController.Update(deltaTime);
    skybox.Update(deltaTime);
    
    HandleInput(deltaTime);
    HandleCameraControls();
    UpdatePlayer(deltaTime);
}

void Game::HandleInput(float deltaTime)
{
    if (IsKeyPressed(KEY_TAB))
    {
        if (IsCursorHidden())
            EnableCursor();
        else
            DisableCursor();
    }
}

void Game::HandleCameraControls()
{
    if (IsKeyPressed(KEY_ONE))
    {
        cameraController.SetMode(CAMERA_MODE_FOLLOW);
        cameraController.SetFollowTarget(&player.position);
    }

    if (IsKeyPressed(KEY_TWO))
        StartOverviewCutscene();

    if (IsKeyPressed(KEY_THREE))
        StartZoomCutscene();

    if (IsKeyPressed(KEY_FOUR))
    {
        cameraController.TransitionTo(
            (Vector3){-10.0f, 8.0f, 0.0f},
            (Vector3){GameConstants::RED_CUBE_POS.x, GameConstants::RED_CUBE_POS.y, GameConstants::RED_CUBE_POS.z},
            1.5f);
        cameraController.SetMode(CAMERA_MODE_FIXED);
    }

    if (IsKeyPressed(KEY_FIVE))
    {
        Vector3 towerTop = {GameConstants::BLUE_TOWER_POS.x, 3.0f, GameConstants::BLUE_TOWER_POS.z};
        StartOrbitCutscene(towerTop, 8.0f, 8);
    }

    if (IsKeyPressed(KEY_SIX))
    {
        cameraController.TransitionTo(
            (Vector3){0.0f, 40.0f, 0.1f},
            GameConstants::WORLD_CENTER,
            2.0f);
        cameraController.SetMode(CAMERA_MODE_FIXED);
    }
}

void Game::UpdatePlayer(float deltaTime)
{
    if (!cameraController.IsCutscenePlaying() && cameraController.mode != CAMERA_MODE_FIXED)
    {
        player.UpdatePlayerMovement(cameraController.camera);
    }
    else if (cameraController.mode == CAMERA_MODE_FIXED)
    {
        UpdatePlayerInFixedCamera(deltaTime);
    }
}

void Game::UpdatePlayerInFixedCamera(float deltaTime)
{
    const float moveAmount = FIXED_CAMERA_MOVE_SPEED * deltaTime;

    Vector3 moveDirection = {0};
    if (IsKeyDown(KEY_W)) moveDirection.z += 1.0f;
    if (IsKeyDown(KEY_S)) moveDirection.z -= 1.0f;
    if (IsKeyDown(KEY_A)) moveDirection.x -= 1.0f;
    if (IsKeyDown(KEY_D)) moveDirection.x += 1.0f;

    if (Vector3Length(moveDirection) > 0.01f)
    {
        moveDirection = Vector3Normalize(moveDirection);
        player.position = Vector3Add(player.position, Vector3Scale(moveDirection, moveAmount));
    }

    // Vertical movement
    if (IsKeyDown(KEY_SPACE) && player.position.y < MAX_VERTICAL_MOVE_HEIGHT)
        player.position.y += moveAmount;
    if (IsKeyDown(KEY_LEFT_SHIFT))
        player.position.y -= moveAmount;
    
    // Clamp to minimum height
    if (player.position.y < MIN_PLAYER_HEIGHT)
        player.position.y = MIN_PLAYER_HEIGHT;
}

void Game::Draw()
{
    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(cameraController.camera);
    DrawScene();
    EndMode3D();

    DrawUI();
    
    EndDrawing();
}

void Game::DrawScene()
{
    skybox.Draw(cameraController.camera);
    
    if (showRaycast)
        player.PlayerRayCast();
        
    DrawPlane(GameConstants::WORLD_CENTER, (Vector2){GameConstants::PLANE_SIZE.x, GameConstants::PLANE_SIZE.y}, LIGHTGRAY);
    
    if (showGrid)
        DrawGrid(GameConstants::GRID_SIZE, 1.0f);

    customModel.drawPlayerModel(player);

    // Draw environment objects
    DrawCube(GameConstants::RED_CUBE_POS, 2.0f, 2.0f, 2.0f, RED);
    DrawCube(GameConstants::BLUE_TOWER_POS, 1.0f, 4.0f, 1.0f, BLUE);
}

void Game::DrawUI()
{
    Draw3DBillboards();
    Draw2DUI();
}

void Game::Draw3DBillboards()
{
    const Vector3 redCubeLabel = {GameConstants::RED_CUBE_POS.x, GameConstants::RED_CUBE_POS.y + 2.0f, GameConstants::RED_CUBE_POS.z};
    const Vector3 blueTowerLabel = {GameConstants::BLUE_TOWER_POS.x, GameConstants::BLUE_TOWER_POS.y + 4.5f, GameConstants::BLUE_TOWER_POS.z};
    const Vector3 playerLabel = Vector3Add(player.position, (Vector3){0.0f, 2.0f, 0.0f});
    
    BillboardText::DrawText3D("Red Cube", redCubeLabel, cameraController.camera, UI_TEXT_SIZE, RED);
    BillboardText::DrawText3DWithBackground("Blue Tower", blueTowerLabel, cameraController.camera, UI_TEXT_SIZE, WHITE, Fade(BLUE, 0.7f));
    BillboardText::DrawText3DScaled("Player", playerLabel, cameraController.camera, 40.0f, 20.0f, GREEN);
    BillboardText::DrawText3DWithLine("Target", GameConstants::WORLD_CENTER, cameraController.camera, 18, YELLOW, ORANGE, 40.0f);
}

void Game::Draw2DUI()
{
    int yPos = UI_MARGIN;
    
    // Controls help
    DrawText("WASD: Move | Mouse: Look | TAB: Toggle Cursor | DELETE: Exit", UI_MARGIN, yPos, UI_TEXT_SIZE, DARKGRAY);
    yPos += UI_LINE_SPACING;
    
    // Camera mode (always shown)
    const char *modeText = "";
    switch (cameraController.mode)
    {
    case CAMERA_MODE_FREE:      modeText = "FREE"; break;
    case CAMERA_MODE_FOLLOW:    modeText = "FOLLOW"; break;
    case CAMERA_MODE_CUTSCENE:  modeText = "CUTSCENE"; break;
    case CAMERA_MODE_FIXED:     modeText = "FIXED"; break;
    }
    DrawText(TextFormat("Camera Mode: %s", modeText), UI_MARGIN, yPos, UI_TEXT_SIZE, GREEN);
    
    // FPS (optional)
    if (showFPS)
    {
        DrawFPS(SCREEN_WIDTH - 100, UI_MARGIN);
    }
    yPos += UI_LINE_SPACING;
    
    // Player position (optional)
    if (showPlayerPos)
    {
        DrawText(TextFormat("Position: X:%.1f Y:%.1f Z:%.1f", 
                 player.position.x, player.position.y, player.position.z),
                 UI_MARGIN, yPos, UI_TEXT_SIZE, DARKGREEN);
        yPos += UI_LINE_SPACING;
    }
    
    // Cutscene indicator
    if (cameraController.IsCutscenePlaying())
    {
        DrawText("[CUTSCENE PLAYING]", UI_MARGIN, yPos, UI_TEXT_SIZE, RED);
        yPos += UI_LINE_SPACING;
    }
    
    yPos += UI_MARGIN;
    
    // Camera controls help
    DrawText("Camera Controls:", UI_MARGIN, yPos, UI_SMALL_TEXT_SIZE, DARKGRAY);
    yPos += UI_LINE_SPACING - 5;
    DrawText("1: Follow Player | 2: Overview | 3: Zoom | 4: Red Cube | 5: Orbit Tower | 6: Bird's Eye", UI_MARGIN, yPos, UI_SMALL_TEXT_SIZE, DARKGRAY);
    
    // Debug menu
    debugMenu.Draw();
}

void Game::Shutdown()
{
    skybox.Unload();

    if (player.modelLoaded)
        UnloadModel(player.model);

    CloseWindow();
}

bool Game::ShouldClose()
{
    return WindowShouldClose() || IsKeyPressed(KEY_DELETE);
}

// Camera cutscene helper methods
void Game::StartOverviewCutscene()
{
    std::vector<CameraWaypoint> cutscene;
    cutscene.push_back({(Vector3){0.0f, 30.0f, 30.0f}, GameConstants::WORLD_CENTER, 3.0f, 60.0f});
    cutscene.push_back({(Vector3){20.0f, 25.0f, 0.0f}, GameConstants::WORLD_CENTER, 3.0f, 50.0f});
    cutscene.push_back({(Vector3){0.0f, 25.0f, -20.0f}, GameConstants::WORLD_CENTER, 3.0f, 50.0f});
    cutscene.push_back({(Vector3){-20.0f, 20.0f, 0.0f}, GameConstants::WORLD_CENTER, 3.0f, 55.0f});
    cameraController.StartCutscene(cutscene);
}

void Game::StartZoomCutscene()
{
    std::vector<CameraWaypoint> cutscene;
    cutscene.push_back({
        Vector3Add(player.position, (Vector3){0.0f, 15.0f, 15.0f}),
        player.position, 2.0f, 70.0f
    });
    cutscene.push_back({
        Vector3Add(player.position, (Vector3){0.0f, 5.0f, 5.0f}),
        player.position, 2.0f, 35.0f
    });
    cutscene.push_back({
        Vector3Add(player.position, (Vector3){3.0f, 2.0f, 3.0f}),
        player.position, 1.5f, 45.0f
    });
    cameraController.StartCutscene(cutscene);
}

void Game::StartOrbitCutscene(const Vector3& target, float radius, int segments)
{
    std::vector<CameraWaypoint> cutscene = CreateCircularOrbit(target, radius, 3.0f, segments, 1.5f, 45.0f);
    cameraController.StartCutscene(cutscene);
}

std::vector<CameraWaypoint> Game::CreateCircularOrbit(const Vector3& center, float radius, float height, int segments, float duration, float fov)
{
    std::vector<CameraWaypoint> waypoints;
    waypoints.reserve(segments);
    
    for (int i = 0; i < segments; i++)
    {
        const float angle = (static_cast<float>(i) / segments) * 2.0f * PI;
        const Vector3 camPos = {
            center.x + radius * cosf(angle),
            center.y + height,
            center.z + radius * sinf(angle)
        };
        waypoints.push_back({camPos, center, duration, fov});
    }
    
    return waypoints;
}
