#include "Game.h"
#include "../graphics/PBRSystem.h"
#include "rlgl.h"

// Constants
namespace GameConstants
{
    constexpr Vector3 WORLD_CENTER = {0.0f, 0.0f, 0.0f};
    constexpr Vector3 PLAYER_START_POS = {0.0f, 0.0f, 0.0f};
    constexpr Vector3 CAMERA_START_POS = {0.0f, 10.0f, 10.0f};
    constexpr float CAMERA_START_FOV = 45.0f;
    constexpr float CAMERA_FOLLOW_DISTANCE = 10.0f;
    constexpr float CAMERA_FOLLOW_HEIGHT = 6.0f;
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
    SetupRenderer(); // Initialize renderer and PBR first
    SetupModels();   // Then setup models that need PBR
    SetupSkybox();
    SetupCollisions();
    SetupDebugMenu();
}

void Game::SetupCamera()
{
    cameraController.Initialize(
        GameConstants::CAMERA_START_POS,
        GameConstants::WORLD_CENTER,
        GameConstants::CAMERA_START_FOV);

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

    // Create PBR test sphere
    Mesh sphereMesh = GenMeshSphere(1.0f, 64, 64);
    pbrTestSphere = LoadModelFromMesh(sphereMesh);

    // Apply PBR shader with material properties
    Vector4 albedo = {0.8f, 0.2f, 0.2f, 1.0f}; // Red color
    float metallic = 0.0f;
    float roughness = 0.3f;
    gPBR.ApplyToModel(pbrTestSphere, albedo, metallic, roughness);
}

void Game::SetupSkybox()
{
    skybox.Load("assets/shader/skybox.vs", "assets/shader/skybox.fs");
    skybox.SetSkyColor({0.0f, 0.0f, 0.0f});
    skybox.SetCloudColor({1.0f, 1.0f, 0.0f});
}

void Game::SetupRenderer()
{
    renderer.Init(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Initialize PBR system
    gPBR.Init();
}

void Game::SetupCollisions()
{
    // Add collision boxes for world objects
    // Red cube at (-4, 1, -4) with size 2x2x2
    collisionSystem.AddBox(GameConstants::RED_CUBE_POS, {2.0f, 2.0f, 2.0f}, "Red Cube", RED);

    // Blue tower at (4, 1, 4) with size 1x4x1
    collisionSystem.AddBox(GameConstants::BLUE_TOWER_POS, {1.0f, 4.0f, 1.0f}, "Blue Tower", BLUE);

    // Add some spherical obstacles
    collisionSystem.AddSphere({-6.0f, 1.5f, 2.0f}, 1.5f, "Ball 1", YELLOW);
    collisionSystem.AddSphere({6.0f, 1.0f, -2.0f}, 1.0f, "Ball 2", ORANGE);

    // Add a capsule obstacle
    collisionSystem.AddCapsule({0.0f, 2.0f, 6.0f}, 0.5f, 3.0f, "Pillar", SKYBLUE);

    // Add a cylinder
    collisionSystem.AddCylinder({-2.0f, 2.0f, -6.0f}, 0.8f, 4.0f, "Column", PURPLE);

    // Add a slope/ramp
    // Ramp at position (8, 0, -8) going upward in the +Z direction
    Vector3 rampCenter = {8.0f, 1.5f, -7.0f};
    float rampWidth = 4.0f;
    float rampLength = 6.0f;
    float rampMaxHeight = 3.0f;
    float rampThickness = 0.5f;

    // Calculate rotation angle for the slope
    // atan2(rise, run) gives us the angle
    float slopeAngle = atan2f(rampMaxHeight, rampLength) * RAD2DEG; // Convert to degrees

    // Main slope collision box - rotated around X axis (pitch)
    collisionSystem.AddBox(
        rampCenter,
        {rampWidth, rampThickness, rampLength},
        "Main Slope",
        Fade(BROWN, 0.5f),
        {slopeAngle, 0, 0} // Pitch rotation
    );

    // Add side walls for the slope
    float slopeWallThickness = 0.3f;
    float slopeWallHeight = rampMaxHeight;

    // Left wall
    collisionSystem.AddBox(
        {rampCenter.x - rampWidth / 2.0f - slopeWallThickness / 2.0f, slopeWallHeight / 2.0f, rampCenter.z},
        {slopeWallThickness, slopeWallHeight, rampLength},
        "Slope Wall Left",
        DARKBROWN);

    // Right wall
    collisionSystem.AddBox(
        {rampCenter.x + rampWidth / 2.0f + slopeWallThickness / 2.0f, slopeWallHeight / 2.0f, rampCenter.z},
        {slopeWallThickness, slopeWallHeight, rampLength},
        "Slope Wall Right",
        DARKBROWN);

    // Add a steep slope (> 45 degrees) to test non-walkable surfaces
    // This will be at position (-8, 1.5, -7)
    Vector3 steepRampCenter = {-8.0f, 2.0f, -7.0f};
    float steepRampWidth = 3.0f;
    float steepRampLength = 4.0f;
    float steepRampHeight = 5.0f; // Steep: 5 units rise over 4 units run = ~51 degrees
    float steepRampThickness = 0.5f;

    float steepSlopeAngle = atan2f(steepRampHeight, steepRampLength) * RAD2DEG;

    // Steep slope collision (> 45 degrees - not walkable, player should slide down)
    collisionSystem.AddBox(
        steepRampCenter,
        {steepRampWidth, steepRampThickness, steepRampLength},
        "Steep Slope (Not Walkable)",
        Fade(MAROON, 0.5f),
        {steepSlopeAngle, 0, 0});

    // Side walls for steep slope
    collisionSystem.AddBox(
        {steepRampCenter.x - steepRampWidth / 2.0f - slopeWallThickness / 2.0f, steepRampHeight / 2.0f, steepRampCenter.z},
        {slopeWallThickness, steepRampHeight, steepRampLength},
        "Steep Slope Wall Left",
        DARKBROWN);

    collisionSystem.AddBox(
        {steepRampCenter.x + steepRampWidth / 2.0f + slopeWallThickness / 2.0f, steepRampHeight / 2.0f, steepRampCenter.z},
        {slopeWallThickness, steepRampHeight, steepRampLength},
        "Steep Slope Wall Right",
        DARKBROWN);

    // World boundaries (invisible walls)
    float worldSize = 16.0f;
    float wallThickness = 1.0f;
    float wallHeight = 10.0f;

    // North wall
    collisionSystem.AddBox({0.0f, wallHeight / 2, -worldSize}, {worldSize * 2, wallHeight, wallThickness}, "North Wall", Fade(GREEN, 0.3f));
    // South wall
    collisionSystem.AddBox({0.0f, wallHeight / 2, worldSize}, {worldSize * 2, wallHeight, wallThickness}, "South Wall", Fade(GREEN, 0.3f));
    // East wall
    collisionSystem.AddBox({worldSize, wallHeight / 2, 0.0f}, {wallThickness, wallHeight, worldSize * 2}, "East Wall", Fade(GREEN, 0.3f));
    // West wall
    collisionSystem.AddBox({-worldSize, wallHeight / 2, 0.0f}, {wallThickness, wallHeight, worldSize * 2}, "West Wall", Fade(GREEN, 0.3f));
}

void Game::SetupDebugMenu()
{
    debugMenu.AddBool("Show Grid", &showGrid);
    debugMenu.AddBool("Show Raycast", &showRaycast);
    debugMenu.AddBool("Show Player Position", &showPlayerPos);
    debugMenu.AddBool("Show FPS", &showFPS);
    debugMenu.AddBool("Show Collision Boxes", &showCollisionBoxes);
    debugMenu.AddBool("Show Player Hitbox", &showPlayerHitbox);
    debugMenu.AddFloat("Jump Strength", &player.jumpStrength, 1.0f, 20.0f, 0.1f);
    debugMenu.AddFloat("Gravity", &player.gravity, -50.0f, -5.0f, 0.1f);
    debugMenu.AddFloat("Sprint Multiplier", &player.sprintMultiplier, 1.0f, 5.0f, 0.1f);
    debugMenu.AddFloat("Collision Radius", &player.collisionRadius, 0.1f, 2.0f, 0.05f);
    debugMenu.AddFloat("Collision Height", &player.collisionHeight, 0.5f, 3.0f, 0.1f);
    debugMenu.AddFloat("Eye Height", &player.eyeHeight, 0.5f, 2.5f, 0.1f);

    std::vector<std::string> modelNames;
    modelNames.reserve(customModel.getModelCount());
    for (int i = 0; i < customModel.getModelCount(); i++)
        modelNames.push_back(customModel.getModelName(i));
    debugMenu.AddString("Player Model", &currentModelIndex, modelNames);

    debugMenu.AddFloat("Camera FOV", &cameraController.camera.fovy, 20.0f, 120.0f, 1.0f);
}

void Game::Update()
{
    float deltaTime = GetFrameTime();

    debugMenu.Update();

    // Update PBR system with camera position
    gPBR.Update(cameraController.camera);

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
            Vector3{-10.0f, 8.0f, 0.0f},
            Vector3{GameConstants::RED_CUBE_POS.x, GameConstants::RED_CUBE_POS.y, GameConstants::RED_CUBE_POS.z},
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
            Vector3{0.0f, 40.0f, 0.1f},
            GameConstants::WORLD_CENTER,
            2.0f);
        cameraController.SetMode(CAMERA_MODE_FIXED);
    }
}

void Game::UpdatePlayer(float deltaTime)
{
    if (!cameraController.IsCutscenePlaying() && cameraController.mode != CAMERA_MODE_FIXED)
    {
        player.UpdatePlayerMovementWithCollision(cameraController.camera, &collisionSystem);
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

    // Draw PBR debug lights
    gPBR.DrawDebugLights();
    EndMode3D();

    DrawUI();

    EndDrawing();
}

void Game::DrawScene()
{
    skybox.Draw(cameraController.camera);

    if (showRaycast)
        player.PlayerRayCast();

    DrawPlane(GameConstants::WORLD_CENTER, {GameConstants::PLANE_SIZE.x, GameConstants::PLANE_SIZE.y}, LIGHTGRAY);

    if (showGrid)
        DrawGrid(GameConstants::GRID_SIZE, 1.0f);

    // Draw PBR test sphere at origin
    DrawModel(pbrTestSphere, (Vector3){0, 1, 0}, 1.0f, WHITE);

    customModel.drawPlayerModel(player);

    // Draw player hitbox for debugging
    if (showPlayerHitbox)
    {
        // DrawCylinderWires draws from base position upward
        DrawCylinderWires(player.position, player.collisionRadius, player.collisionRadius, player.collisionHeight, 16, LIME);

        // Draw a sphere at the player's actual position (feet)
        DrawSphereWires(player.position, 0.15f, 8, 8, RED);
        // Draw a sphere at the top of the hitbox
        DrawSphereWires({player.position.x, player.position.y + player.collisionHeight, player.position.z}, 0.15f, 8, 8, BLUE);
    }

    // Draw environment objects
    DrawCube(GameConstants::RED_CUBE_POS, 2.0f, 2.0f, 2.0f, RED);
    DrawCube(GameConstants::BLUE_TOWER_POS, 1.0f, 4.0f, 1.0f, BLUE);

    // Draw additional collision objects
    DrawSphere({-6.0f, 1.5f, 2.0f}, 1.5f, YELLOW);
    DrawSphere({6.0f, 1.0f, -2.0f}, 1.0f, ORANGE);

    // Draw capsule as cylinder with spheres on ends
    Vector3 capsulePos = {0.0f, 2.0f, 6.0f};
    DrawCylinder(capsulePos, 0.5f, 0.5f, 3.0f, 16, SKYBLUE);

    // Draw cylinder
    Vector3 cylinderPos = {-2.0f, 0.0f, -6.0f};
    DrawCylinder(cylinderPos, 0.8f, 0.8f, 4.0f, 16, PURPLE);

    // Draw slope/ramp with mesh
    Vector3 rampCenter = {8.0f, 1.5f, -7.0f};
    float rampWidth = 4.0f;
    float rampLength = 6.0f;
    float rampMaxHeight = 3.0f;
    float rampThickness = 0.5f;
    float slopeWallThickness = 0.3f;

    // Calculate slope angle
    float slopeAngle = atan2f(rampMaxHeight, rampLength) * RAD2DEG;

    // Draw the angled slope surface
    rlPushMatrix();
    rlTranslatef(rampCenter.x, rampCenter.y, rampCenter.z);
    rlRotatef(slopeAngle, 1.0f, 0.0f, 0.0f); // Pitch rotation
    DrawCube({0.0f, 0.0f, 0.0f}, rampWidth, rampThickness, rampLength, BROWN);
    rlPopMatrix();

    // Draw side walls
    // Left wall
    DrawCube(
        {rampCenter.x - rampWidth / 2.0f - slopeWallThickness / 2.0f, rampMaxHeight / 2.0f, rampCenter.z},
        slopeWallThickness, rampMaxHeight, rampLength,
        DARKBROWN);

    // Right wall
    DrawCube(
        {rampCenter.x + rampWidth / 2.0f + slopeWallThickness / 2.0f, rampMaxHeight / 2.0f, rampCenter.z},
        slopeWallThickness, rampMaxHeight, rampLength,
        DARKBROWN);

    // Draw steep slope (> 45 degrees)
    Vector3 steepRampCenter = {-8.0f, 2.0f, -7.0f};
    float steepRampWidth = 3.0f;
    float steepRampLength = 4.0f;
    float steepRampHeight = 5.0f;
    float steepRampThickness = 0.5f;

    float steepSlopeAngle = atan2f(steepRampHeight, steepRampLength) * RAD2DEG;

    // Draw the steep angled slope surface
    rlPushMatrix();
    rlTranslatef(steepRampCenter.x, steepRampCenter.y, steepRampCenter.z);
    rlRotatef(steepSlopeAngle, 1.0f, 0.0f, 0.0f);
    DrawCube({0.0f, 0.0f, 0.0f}, steepRampWidth, steepRampThickness, steepRampLength, MAROON);
    rlPopMatrix();

    // Draw steep slope side walls
    DrawCube(
        {steepRampCenter.x - steepRampWidth / 2.0f - slopeWallThickness / 2.0f, steepRampHeight / 2.0f, steepRampCenter.z},
        slopeWallThickness, steepRampHeight, steepRampLength,
        DARKBROWN);

    DrawCube(
        {steepRampCenter.x + steepRampWidth / 2.0f + slopeWallThickness / 2.0f, steepRampHeight / 2.0f, steepRampCenter.z},
        slopeWallThickness, steepRampHeight, steepRampLength,
        DARKBROWN);

    // Draw collision boxes for debugging
    if (showCollisionBoxes)
        collisionSystem.DrawDebug(false);
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
    const Vector3 playerLabel = Vector3Add(player.position, {0.0f, 2.0f, 0.0f});

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
    gPBR.Shutdown();
    renderer.Shutdown();
    skybox.Unload();

    if (player.modelLoaded)
        UnloadModel(player.model);

    UnloadModel(pbrTestSphere);

    CloseWindow();
}

bool Game::ShouldClose()
{
    return WindowShouldClose() || IsKeyPressed(KEY_DELETE);
}

// Camera cutscene helper methods
void Game::StartOverviewCutscene()
{
    std::vector<CameraWaypoint> cutscene = {
        {{0.0f, 30.0f, 30.0f}, GameConstants::WORLD_CENTER, 3.0f, 60.0f},
        {{20.0f, 25.0f, 0.0f}, GameConstants::WORLD_CENTER, 3.0f, 50.0f},
        {{0.0f, 25.0f, -20.0f}, GameConstants::WORLD_CENTER, 3.0f, 50.0f},
        {{-20.0f, 20.0f, 0.0f}, GameConstants::WORLD_CENTER, 3.0f, 55.0f}};
    cameraController.StartCutscene(cutscene);
}

void Game::StartZoomCutscene()
{
    std::vector<CameraWaypoint> cutscene = {
        {Vector3Add(player.position, {0.0f, 15.0f, 15.0f}), player.position, 2.0f, 70.0f},
        {Vector3Add(player.position, {0.0f, 5.0f, 5.0f}), player.position, 2.0f, 35.0f},
        {Vector3Add(player.position, {3.0f, 2.0f, 3.0f}), player.position, 1.5f, 45.0f}};
    cameraController.StartCutscene(cutscene);
}

void Game::StartOrbitCutscene(const Vector3 &target, float radius, int segments)
{
    std::vector<CameraWaypoint> cutscene = CreateCircularOrbit(target, radius, 3.0f, segments, 1.5f, 45.0f);
    cameraController.StartCutscene(cutscene);
}

std::vector<CameraWaypoint> Game::CreateCircularOrbit(const Vector3 &center, float radius, float height, int segments, float duration, float fov)
{
    std::vector<CameraWaypoint> waypoints;
    waypoints.reserve(segments);

    for (int i = 0; i < segments; i++)
    {
        const float angle = (static_cast<float>(i) / segments) * 2.0f * PI;
        const Vector3 camPos = {
            center.x + radius * cosf(angle),
            center.y + height,
            center.z + radius * sinf(angle)};
        waypoints.push_back({camPos, center, duration, fov});
    }

    return waypoints;
}
