#include "Game.h"
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
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Zelda-like 3D Game Structure skid");
    SetTargetFPS(TARGET_FPS);
    SetExitKey(KEY_NULL);

    // Start with cursor enabled for main menu
    EnableCursor();

    // Initialize menus first
    SetupMenus();

    SetupCamera();
    SetupPlayer();
    SetupRenderer(); // Initialize renderer and PBR first
    SetupModels();   // Then setup models that need PBR
    SetupSkybox();
    SetupGrass();
    SetupWater();
    SetupCollisions();
    SetupNPCs();
    SetupDebugMenu();
    SetupPostProcessingMenu();

    // Start in main menu state
    currentState = GameState::MAIN_MENU;
}

void Game::SetupMenus()
{
    // Setup main menu callbacks using current window size so menus scale correctly
    mainMenu.Init(GetScreenWidth(), GetScreenHeight(), [this]()
                  { ChangeState(GameState::PLAYING); }, // Play
                  [this]()
                  { ChangeState(GameState::SETTINGS); }, // Settings
                  [this]()
                  { ChangeState(GameState::QUIT); } // Quit
    );

    // Setup pause menu callbacks
    pauseMenu.Init(GetScreenWidth(), GetScreenHeight(), [this]()
                   { ChangeState(GameState::PLAYING); }, // Resume
                   [this]()
                   { ChangeState(GameState::SETTINGS); }, // Settings
                   [this]()
                   { ChangeState(GameState::MAIN_MENU); }, // Main Menu
                   [this]()
                   { ChangeState(GameState::QUIT); } // Quit
    );
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

    // Create PBR models for world objects
    pbrRedCube = LoadModelFromMesh(GenMeshCube(2.0f, 2.0f, 2.0f));
    pbrBlueTower = LoadModelFromMesh(GenMeshCube(1.0f, 4.0f, 1.0f));
    pbrYellowSphere = LoadModelFromMesh(GenMeshSphere(1.5f, 32, 32));
    pbrOrangeSphere = LoadModelFromMesh(GenMeshSphere(1.0f, 32, 32));
    pbrCapsule = LoadModelFromMesh(GenMeshCylinder(0.5f, 3.0f, 16));
    pbrCylinder = LoadModelFromMesh(GenMeshCylinder(0.8f, 4.0f, 16));
    pbrGroundPlane = LoadModelFromMesh(GenMeshPlane(GameConstants::PLANE_SIZE.x, GameConstants::PLANE_SIZE.y, 10, 10));
    pbrRamp = LoadModelFromMesh(GenMeshCube(4.0f, 0.5f, 6.0f));
    pbrSteepRamp = LoadModelFromMesh(GenMeshCube(3.0f, 0.5f, 4.0f));

    // Apply shaders to all models using RenderManager (centralized)
    std::vector<RenderManager::ModelMaterial> models = {
        {&pbrTestSphere, {0.8f, 0.2f, 0.2f, 1.0f}, 1.0f, 0.3f},
        {&pbrRedCube, {0.8f, 0.1f, 0.1f, 1.0f}, 0.2f, 0.4f},
        {&pbrBlueTower, {0.1f, 0.2f, 0.8f, 1.0f}, 0.0f, 0.3f},
        {&pbrYellowSphere, {0.9f, 0.9f, 0.2f, 1.0f}, 0.0f, 0.6f},
        {&pbrOrangeSphere, {0.9f, 0.5f, 0.1f, 1.0f}, 0.1f, 0.3f},
        {&pbrCapsule, {0.3f, 0.7f, 0.9f, 1.0f}, 0.0f, 0.5f},
        {&pbrCylinder, {0.5f, 0.2f, 0.8f, 1.0f}, 0.0f, 0.4f},
        {&pbrGroundPlane, {0.3f, 0.3f, 0.3f, 1.0f}, 0.0f, 0.8f},
        {&pbrRamp, {0.4f, 0.25f, 0.15f, 1.0f}, 0.0f, 0.7f},
        {&pbrSteepRamp, {0.5f, 0.1f, 0.1f, 1.0f}, 0.0f, 0.6f}};
    renderManager.ApplyShaders(models);

    // Register models with GeometryRenderer for GPU culling
    GeometryRenderer *geoRenderer = renderManager.GetGeometryRenderer();
    modelID_TestSphere = geoRenderer->RegisterModel("TestSphere", &pbrTestSphere);
    modelID_RedCube = geoRenderer->RegisterModel("RedCube", &pbrRedCube);
    modelID_BlueTower = geoRenderer->RegisterModel("BlueTower", &pbrBlueTower);
    modelID_YellowSphere = geoRenderer->RegisterModel("YellowSphere", &pbrYellowSphere);
    modelID_OrangeSphere = geoRenderer->RegisterModel("OrangeSphere", &pbrOrangeSphere);
    modelID_Capsule = geoRenderer->RegisterModel("Capsule", &pbrCapsule);
    modelID_Cylinder = geoRenderer->RegisterModel("Cylinder", &pbrCylinder);
    modelID_GroundPlane = geoRenderer->RegisterModel("GroundPlane", &pbrGroundPlane);
    modelID_Ramp = geoRenderer->RegisterModel("Ramp", &pbrRamp);
    modelID_SteepRamp = geoRenderer->RegisterModel("SteepRamp", &pbrSteepRamp);

    // Create world entities
    using namespace World;

    // Ground plane - render only
    {
        Entity e = world.CreateEntity();
        world.AddMetadata(e, "GroundPlane", true);
        world.AddTransform(e, GameConstants::WORLD_CENTER);
        world.AddRender(e, &pbrGroundPlane, modelID_GroundPlane, {77, 77, 77, 255}, 0.0f, 0.8f);
    }

    // Test sphere at origin
    {
        Entity e = world.CreateEntity();
        world.AddMetadata(e, "TestSphere", true);
        world.AddTransform(e, {0.0f, 1.0f, 0.0f});
        world.AddRender(e, &pbrTestSphere, modelID_TestSphere, {204, 51, 51, 255}, 1.0f, 0.3f);
    }

    // Red cube - render + collision
    {
        Entity e = world.CreateEntity();
        world.AddMetadata(e, "RedCube", true);
        world.AddTransform(e, GameConstants::RED_CUBE_POS);
        world.AddRender(e, &pbrRedCube, modelID_RedCube, {204, 26, 26, 255}, 0.2f, 0.4f);
        world.AddCollision(e, COLLISION_BOX, {2.0f, 2.0f, 2.0f}, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f}, RED);
    }

    // Blue tower - render + collision
    {
        Entity e = world.CreateEntity();
        world.AddMetadata(e, "BlueTower", true);
        world.AddTransform(e, GameConstants::BLUE_TOWER_POS);
        world.AddRender(e, &pbrBlueTower, modelID_BlueTower, {26, 51, 204, 255}, 0.0f, 0.3f);
        world.AddCollision(e, COLLISION_BOX, {1.0f, 4.0f, 1.0f}, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f}, BLUE);
    }

    // Yellow sphere - render + collision
    {
        Entity e = world.CreateEntity();
        world.AddMetadata(e, "YellowSphere", true);
        world.AddTransform(e, {-6.0f, 1.5f, 2.0f});
        world.AddRender(e, &pbrYellowSphere, modelID_YellowSphere, {230, 230, 51, 255}, 0.0f, 0.6f);
        world.AddCollision(e, COLLISION_SPHERE, {0.0f, 0.0f, 0.0f}, 1.5f, 0.0f, {0.0f, 0.0f, 0.0f}, YELLOW);
    }

    // Orange sphere - render + collision
    {
        Entity e = world.CreateEntity();
        world.AddMetadata(e, "OrangeSphere", true);
        world.AddTransform(e, {6.0f, 1.0f, -2.0f});
        world.AddRender(e, &pbrOrangeSphere, modelID_OrangeSphere, {230, 128, 26, 255}, 0.1f, 0.3f);
        world.AddCollision(e, COLLISION_SPHERE, {0.0f, 0.0f, 0.0f}, 1.0f, 0.0f, {0.0f, 0.0f, 0.0f}, ORANGE);
    }

    // Capsule pillar - render + collision
    {
        Entity e = world.CreateEntity();
        world.AddMetadata(e, "Capsule", true);
        world.AddTransform(e, {0.0f, 2.0f, 6.0f});
        world.AddRender(e, &pbrCapsule, modelID_Capsule, {77, 179, 230, 255}, 0.0f, 0.5f);
        world.AddCollision(e, COLLISION_CAPSULE, {0.0f, 0.0f, 0.0f}, 0.5f, 3.0f, {0.0f, 0.0f, 0.0f}, SKYBLUE);
    }

    // Cylinder column - render + collision
    {
        Entity e = world.CreateEntity();
        world.AddMetadata(e, "Cylinder", true);
        world.AddTransform(e, {-2.0f, 2.0f, -6.0f});
        world.AddRender(e, &pbrCylinder, modelID_Cylinder, {128, 51, 204, 255}, 0.0f, 0.4f);
        world.AddCollision(e, COLLISION_CYLINDER, {0.0f, 0.0f, 0.0f}, 0.8f, 4.0f, {0.0f, 0.0f, 0.0f}, PURPLE);
    }

    // Main ramp - collision only (visual shown via debug collision boxes)
    {
        Vector3 rampCenter = {8.0f, 1.5f, -7.0f};
        float rampWidth = 4.0f;
        float rampLength = 6.0f;
        float rampMaxHeight = 3.0f;
        float slopeAngle = atan2f(rampMaxHeight, rampLength) * RAD2DEG;

        Entity e = world.CreateEntity();
        world.AddMetadata(e, "MainRamp", true);
        world.AddTransform(e, rampCenter);
        world.AddCollision(e, COLLISION_BOX, {rampWidth, 0.5f, rampLength}, 0.0f, 0.0f, {slopeAngle, 0.0f, 0.0f}, Fade(BROWN, 0.5f));

        // Side walls for main ramp
        float slopeWallThickness = 0.3f;
        Entity leftWall = world.CreateEntity();
        world.AddMetadata(leftWall, "Slope Wall Left", true);
        world.AddTransform(leftWall, {rampCenter.x - rampWidth / 2.0f - slopeWallThickness / 2.0f, rampMaxHeight / 2.0f, rampCenter.z});
        world.AddCollision(leftWall, COLLISION_BOX, {slopeWallThickness, rampMaxHeight, rampLength}, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f}, DARKBROWN);

        Entity rightWall = world.CreateEntity();
        world.AddMetadata(rightWall, "Slope Wall Right", true);
        world.AddTransform(rightWall, {rampCenter.x + rampWidth / 2.0f + slopeWallThickness / 2.0f, rampMaxHeight / 2.0f, rampCenter.z});
        world.AddCollision(rightWall, COLLISION_BOX, {slopeWallThickness, rampMaxHeight, rampLength}, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f}, DARKBROWN);
    }

    // Steep ramp - collision only (visual shown via debug collision boxes)
    {
        Vector3 steepRampCenter = {-8.0f, 2.0f, -7.0f};
        float steepRampWidth = 3.0f;
        float steepRampLength = 4.0f;
        float steepRampHeight = 5.0f;
        float steepSlopeAngle = atan2f(steepRampHeight, steepRampLength) * RAD2DEG;

        Entity e = world.CreateEntity();
        world.AddMetadata(e, "SteepRamp", true);
        world.AddTransform(e, steepRampCenter);
        world.AddCollision(e, COLLISION_BOX, {steepRampWidth, 0.5f, steepRampLength}, 0.0f, 0.0f, {steepSlopeAngle, 0.0f, 0.0f}, Fade(MAROON, 0.5f));

        // Side walls for steep ramp
        float slopeWallThickness = 0.3f;
        Entity leftWall = world.CreateEntity();
        world.AddMetadata(leftWall, "Steep Slope Wall Left", true);
        world.AddTransform(leftWall, {steepRampCenter.x - steepRampWidth / 2.0f - slopeWallThickness / 2.0f, steepRampHeight / 2.0f, steepRampCenter.z});
        world.AddCollision(leftWall, COLLISION_BOX, {slopeWallThickness, steepRampHeight, steepRampLength}, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f}, DARKBROWN);

        Entity rightWall = world.CreateEntity();
        world.AddMetadata(rightWall, "Steep Slope Wall Right", true);
        world.AddTransform(rightWall, {steepRampCenter.x + steepRampWidth / 2.0f + slopeWallThickness / 2.0f, steepRampHeight / 2.0f, steepRampCenter.z});
        world.AddCollision(rightWall, COLLISION_BOX, {slopeWallThickness, steepRampHeight, steepRampLength}, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f}, DARKBROWN);
    }

    TraceLog(LOG_INFO, "Game: Models setup complete with %d entities", world.GetEntityCount());
}

void Game::SetupSkybox()
{
    renderManager.InitializeSkybox("assets/shader/skybox.vs", "assets/shader/skybox.fs");
    renderManager.ConfigureSkybox({0.5f, 0.5f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.95f, 0.8f});
}

void Game::SetupRenderer()
{
    // Initialize render manager - single entry point for all rendering
    renderManager.Init(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Setup sun direction
    Vector3 sunDirection = {0.3f, 0.5f, 0.8f};
    renderManager.SetSunDirection(sunDirection);

    // Create lights (used by the PBR renderer)
    renderManager.CreateDirectionalLight(sunDirection, {1.0f, 0.95f, 0.8f, 1.0f}, 2.0f);
    renderManager.CreatePointLight({-5.0f, 4.0f, -5.0f}, {1.0f, 0.9f, 0.8f, 1.0f}, 12.0f, 15.0f);
    renderManager.CreatePointLight({5.0f, 4.0f, 5.0f}, {0.8f, 0.9f, 1.0f, 1.0f}, 12.0f, 15.0f);
    renderManager.CreatePointLight({0.0f, 6.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 15.0f, 20.0f);

    TraceLog(LOG_INFO, "Game: RenderManager initialized with all lights");
}

void Game::SetupGrass()
{
    // Initialize grass renderer with 200000 grass blades over a 30x30 area
    renderManager.InitializeGrass(200000, 30.0f);
    renderManager.ConfigureGrass({1.0f, 0.5f}, 0.5f, 2.0f);
}

void Game::SetupWater()
{
    // Initialize water renderer with a large water plane beneath the map
    renderManager.InitializeWater(50.0f, 50.0f, -0.5f);
}

void Game::SetupCollisions()
{
    // Populate collision system from entities
    const auto &transforms = world.GetTransforms();
    const auto &collisions = world.GetCollisions();
    const auto &metadata = world.GetMetadata();

    // Get all entities with Transform + Collision components
    auto entities = world.GetEntitiesWithComponents(World::COMPONENT_TRANSFORM | World::COMPONENT_COLLISION);

    for (World::Entity e : entities)
    {
        int transformIdx = world.GetTransformIndex(e);
        int collisionIdx = world.GetCollisionIndex(e);
        int metadataIdx = world.GetMetadataIndex(e);

        if (transformIdx < 0 || collisionIdx < 0)
            continue;

        const Vector3 &pos = transforms.positions[transformIdx];
        CollisionShape shape = collisions.shapes[collisionIdx];
        const Vector3 &size = collisions.sizes[collisionIdx];
        float radius = collisions.radii[collisionIdx];
        float height = collisions.heights[collisionIdx];
        const Vector3 &rotation = collisions.rotations[collisionIdx];
        Color debugColor = collisions.debugColors[collisionIdx];
        std::string name = (metadataIdx >= 0) ? metadata.names[metadataIdx] : "Unknown";

        // Add to collision system based on shape type
        switch (shape)
        {
        case COLLISION_BOX:
            collisionSystem.AddBox(pos, size, name, debugColor, rotation);
            break;
        case COLLISION_SPHERE:
            collisionSystem.AddSphere(pos, radius, name, debugColor);
            break;
        case COLLISION_CAPSULE:
            collisionSystem.AddCapsule(pos, radius, height, name, debugColor);
            break;
        case COLLISION_CYLINDER:
            collisionSystem.AddCylinder(pos, radius, height, name, debugColor);
            break;
        }
    }

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

void Game::SetupNPCs()
{
    // Create some NPCs with different dialogue
    std::vector<std::string> dialogue1 = {
        "Hello, adventurer!",
        "Nice weather today!",
        "Have you seen the tower?"};
    npcs.push_back(NPC({5.0f, 0.0f, 5.0f}, "Bob", dialogue1, BLUE));

    std::vector<std::string> dialogue2 = {
        "Greetings!",
        "This world is amazing!",
        "Try jumping around!"};
    npcs.push_back(NPC({-5.0f, 0.0f, -5.0f}, "Alice", dialogue2, PURPLE));

    std::vector<std::string> dialogue3 = {
        "Watch out for slopes!",
        "Press E to talk!",
        "I love speech bubbles!"};
    npcs.push_back(NPC({0.0f, 0.0f, 8.0f}, "Charlie", dialogue3, GREEN));

    // Initialize speech bubble manager
    speechBubbleManager = Graphics::SpeechBubbleManager(10);
}

void Game::SetupDebugMenu()
{
    debugMenu.AddBool("Show Grid", &showGrid);
    debugMenu.AddBool("Show Raycast", &showRaycast);
    debugMenu.AddBool("Show Player Position", &showPlayerPos);
    debugMenu.AddBool("Show FPS", &showFPS);
    debugMenu.AddBool("Show Collision Boxes", &showCollisionBoxes);
    debugMenu.AddBool("Show Player Hitbox", &showPlayerHitbox);
    debugMenu.AddBool("Show Grass", &showGrass);
    debugMenu.AddFloat("Jump Strength", &player.jumpStrength, 1.0f, 20.0f, 0.1f);
    debugMenu.AddFloat("Gravity", &player.gravity, -50.0f, -5.0f, 0.1f);
    debugMenu.AddFloat("Sprint Multiplier", &player.sprintMultiplier, 1.0f, 5.0f, 0.1f);
    debugMenu.AddFloat("Collision Radius", &player.collisionRadius, 0.1f, 2.0f, 0.05f);
    debugMenu.AddFloat("Collision Height", &player.collisionHeight, 0.5f, 3.0f, 0.1f);
    debugMenu.AddFloat("Eye Height", &player.eyeHeight, 0.5f, 2.5f, 0.1f);
    debugMenu.AddFloat("Interaction Range", &npcInteractionRange, 1.0f, 10.0f, 0.5f);
    debugMenu.AddFloat("Geometry Cull Margin", &geometryCullMargin, 1.0f, 2.0f, 0.05f);
    debugMenu.AddFloat("Grass Cull Margin", &grassCullMargin, 1.0f, 2.0f, 0.05f);

    // Geometry culling aggressiveness (radius multiplier)
    std::vector<std::string> modelNames;
    modelNames.reserve(customModel.getModelCount());
    for (int i = 0; i < customModel.getModelCount(); i++)
        modelNames.push_back(customModel.getModelName(i));
    debugMenu.AddString("Player Model", &currentModelIndex, modelNames);

    // Note: Display Mode moved to Settings menu (not debug menu)

    debugMenu.AddFloat("Camera FOV", &cameraController.camera.fovy, 20.0f, 120.0f, 1.0f);
}

void Game::SetupPostProcessingMenu()
{
    postProcessingMenu.AddBool("Enable Post-Processing", &enablePostProcessing);
    postProcessingMenu.AddBool("Enable Grayscale", &enableGrayscale);
}

void Game::Update()
{
    // Apply any pending display mode changes requested by menus
    ApplyDisplayModeIfChanged();

    // Handle window resize in a single place
    HandleWindowResize();

    switch (currentState)
    {
    case GameState::MAIN_MENU:
        UpdateMainMenu();
        break;
    case GameState::PLAYING:
        UpdatePlaying();
        break;
    case GameState::PAUSED:
        UpdatePaused();
        break;
    case GameState::SETTINGS:
        UpdateSettings();
        break;
    case GameState::QUIT:
        // Will be handled in ShouldClose()
        break;
    }
}

void Game::UpdateMainMenu()
{
    mainMenu.Update();
}

void Game::UpdatePlaying()
{
    float deltaTime = GetFrameTime();

    // Check for pause
    if (IsKeyPressed(KEY_ESCAPE))
    {
        ChangeState(GameState::PAUSED);
        return;
    }

    debugMenu.Update();
    postProcessingMenu.Update();

    // Apply player model selection from debug menu
    if (currentModelIndex != previousModelIndex)
    {
        int modelCount = customModel.getModelCount();
        if (modelCount > 0)
        {
            // Wrap index to valid range in case debug menu underflows/overflows
            int clampedIndex = currentModelIndex % modelCount;
            if (clampedIndex < 0)
                clampedIndex += modelCount;

            currentModelIndex = clampedIndex;
            customModel.loadPlayerModel(player, currentModelIndex);
            previousModelIndex = currentModelIndex;
        }
    }

    // Update rendering through unified RenderManager
    renderManager.UpdateCamera(cameraController.camera);

    // Sync all render settings to the manager (single source of truth)
    renderManager.EnablePostProcessing(enablePostProcessing);
    renderManager.EnableGrayscale(enableGrayscale);

    // display mode is handled globally in ApplyDisplayModeIfChanged()

    cameraController.Update(deltaTime);
    renderManager.UpdateGrass(deltaTime, cameraController.camera);
    renderManager.UpdateWater(deltaTime, cameraController.camera);
    renderManager.UpdateSkybox(deltaTime);

    // Populate geometry instances (cleared and rebuilt each frame for dynamic culling)
    GeometryRenderer *geoRenderer = renderManager.GetGeometryRenderer();
    geoRenderer->ClearInstances();
    geoRenderer->SetCullingRadiusMultiplier(geometryCullMargin);
    renderManager.GetGrassRenderer()->SetCullingRadiusMultiplier(grassCullMargin);

    // Add all renderable entities
    const auto &transforms = world.GetTransforms();
    const auto &renders = world.GetRenders();

    auto renderableEntities = world.GetEntitiesWithComponents(World::COMPONENT_TRANSFORM | World::COMPONENT_RENDER);

    for (World::Entity e : renderableEntities)
    {
        int transformIdx = world.GetTransformIndex(e);
        int renderIdx = world.GetRenderIndex(e);

        if (transformIdx >= 0 && renderIdx >= 0)
        {
            const Vector3 &pos = transforms.positions[transformIdx];
            const Vector3 &scale = transforms.scales[transformIdx];
            int geoModelID = renders.geometryModelIDs[renderIdx];

            geoRenderer->AddInstance(geoModelID, pos, scale.x); // Uniform scale
        }
    }

    // Update geometry renderer to perform culling
    renderManager.UpdateGeometry(deltaTime, cameraController.camera);

    // Update NPCs
    for (auto &npc : npcs)
    {
        npc.SetInteractionRange(npcInteractionRange); // Sync with global setting
        npc.Update(player.position);
    }

    speechBubbleManager.UpdateAll(deltaTime);

    // Handle NPC interaction (E key to talk)
    // Interaction requires:
    // 1. Player within interaction radius (npc.IsInteractable())
    // 2. Player looking at NPC (Raycast hit)
    // 3. No active speech bubbles (Spam prevention)
    if (IsKeyPressed(KEY_E))
    {
        if (speechBubbleManager.GetActiveCount() == 0)
        {
            Ray playerRay = player.GetForwardRay();

            for (auto &npc : npcs)
            {
                // Check distance (IsInteractable)
                if (npc.IsInteractable())
                {
                    // Check if player is facing NPC (Dot Product)
                    Vector3 toNpc = Vector3Subtract(npc.GetPosition(), player.position);
                    toNpc.y = 0; // Ignore height difference

                    if (Vector3LengthSqr(toNpc) > 0.001f)
                    {
                        toNpc = Vector3Normalize(toNpc);
                        Vector3 playerDir = {playerRay.direction.x, 0, playerRay.direction.z}; // Player body forward

                        // Check alignment (dot > 0.5 means roughly within 60 degrees cone)
                        float dot = Vector3DotProduct(playerDir, toNpc);

                        if (dot > 0.5f)
                        {
                            std::string dialogue = npc.GetNextDialogue();
                            // Pass reference to position so bubble follows NPC
                            speechBubbleManager.ShowBubble(dialogue, npc.GetPosition(), &npc.GetPositionRef(), 4.0f);
                            break; // Interact with only one
                        }
                    }
                }
            }
        }
    }

    HandleInput(deltaTime);
    HandleCameraControls();
    UpdatePlayer(deltaTime);
}

void Game::UpdatePaused()
{
    pauseMenu.Update();

    // Also allow ESC to resume
    if (IsKeyPressed(KEY_ESCAPE))
    {
        ChangeState(GameState::PLAYING);
    }
}

void Game::ChangeState(GameState newState)
{
    GameState oldState = currentState;
    currentState = newState;

    // Handle state transitions
    switch (newState)
    {
    case GameState::MAIN_MENU:
        EnableCursor();
        mainMenu.Reset();
        break;
    case GameState::PLAYING:
        DisableCursor();
        break;
    case GameState::PAUSED:
        EnableCursor();
        break;
    case GameState::SETTINGS:
        EnableCursor();
        // Remember where we came from so Back returns to the appropriate state
        settingsReturnState = oldState;
        // Initialize settings menu with pointer to fullscreenMode and Back -> previous state
        settingsMenu.Init(GetScreenWidth(), GetScreenHeight(), &fullscreenMode, [this]()
                          { ChangeState(settingsReturnState); });
        break;
    case GameState::QUIT:
        // Nothing special needed
        break;
    }
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

void Game::UpdateSettings()
{
    settingsMenu.Update();
}

void Game::ApplyRenderingMode()
{
    // Prepare all models with their materials
    std::vector<RenderManager::ModelMaterial> models = {
        {&pbrTestSphere, {0.8f, 0.2f, 0.2f, 1.0f}, 0.0f, 0.3f},
        {&pbrRedCube, {0.8f, 0.1f, 0.1f, 1.0f}, 0.2f, 0.4f},
        {&pbrBlueTower, {0.1f, 0.2f, 0.8f, 1.0f}, 0.0f, 0.3f},
        {&pbrYellowSphere, {0.9f, 0.9f, 0.2f, 1.0f}, 0.0f, 0.6f},
        {&pbrOrangeSphere, {0.9f, 0.5f, 0.1f, 1.0f}, 0.1f, 0.3f},
        {&pbrCapsule, {0.3f, 0.7f, 0.9f, 1.0f}, 0.0f, 0.5f},
        {&pbrCylinder, {0.5f, 0.2f, 0.8f, 1.0f}, 0.0f, 0.4f},
        {&pbrGroundPlane, {0.3f, 0.3f, 0.3f, 1.0f}, 0.0f, 0.8f},
        {&pbrRamp, {0.4f, 0.25f, 0.15f, 1.0f}, 0.0f, 0.7f},
        {&pbrSteepRamp, {0.5f, 0.1f, 0.1f, 1.0f}, 0.0f, 0.6f}};

    // Apply all shaders at once through RenderManager
    renderManager.ApplyShaders(models);
}

void Game::DrawSettings()
{
    settingsMenu.Draw();
}

void Game::ApplyDisplayModeIfChanged()
{
    if (fullscreenMode == previousFullscreenMode)
        return;

    // Use RenderManager to handle all display mode changes
    renderManager.ApplyDisplayMode(fullscreenMode);
    previousFullscreenMode = fullscreenMode;

    // Reinitialize menus so their layouts update to the new window size
    SetupMenus();

    // If currently in settings, reinit settings menu to match new size
    if (currentState == GameState::SETTINGS)
    {
        settingsMenu.Init(GetScreenWidth(), GetScreenHeight(), &fullscreenMode, [this]()
                          { ChangeState(settingsReturnState); });
    }
}

void Game::HandleWindowResize()
{
    if (!IsWindowResized())
        return;

    const int newWidth = GetScreenWidth();
    const int newHeight = GetScreenHeight();

    renderManager.SetWindowSize(newWidth, newHeight);

    // Rebuild menus so layout matches the new window size
    SetupMenus();

    if (currentState == GameState::SETTINGS)
    {
        settingsMenu.Init(newWidth, newHeight, &fullscreenMode, [this]()
                          { ChangeState(settingsReturnState); });
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

    switch (currentState)
    {
    case GameState::MAIN_MENU:
        ClearBackground(RAYWHITE);
        DrawMainMenu();
        break;
    case GameState::PLAYING:
        DrawPlaying(); // Post-processing handles its own clearing
        break;
    case GameState::PAUSED:
        DrawPlaying(); // Draw game in background
        DrawPaused();  // Draw pause overlay on top
        break;
    case GameState::SETTINGS:
        // Draw game behind the settings UI
        DrawPlaying();
        DrawSettings();
        break;
    case GameState::QUIT:
        break;
    }

    EndDrawing();
}

void Game::DrawMainMenu()
{
    mainMenu.Draw();
}

void Game::DrawPlaying()
{
    // Use RenderManager for all rendering - single unified entry point
    renderManager.BeginFrame();

    // Render 3D scene using a lambda callback
    renderManager.RenderScene([this]()
                              {
        BeginMode3D(cameraController.camera);
        DrawScene();
        renderManager.GetLightRenderer()->DrawDebugLights();
        EndMode3D(); }, cameraController.camera);

    renderManager.EndFrame();

    // Draw UI on top
    DrawUI();
}

void Game::DrawPaused()
{
    pauseMenu.Draw();
}

void Game::DrawScene()
{
    renderManager.GetSkyboxRenderer()->Draw(cameraController.camera);

    if (showRaycast)
        player.PlayerRayCast();

    // Draw PBR ground plane
    DrawModel(pbrGroundPlane, GameConstants::WORLD_CENTER, 1.0f, WHITE);

    if (showGrid)
        DrawGrid(GameConstants::GRID_SIZE, 1.0f);

    // Draw PBR test sphere at origin
    DrawModel(pbrTestSphere, Vector3{0, 1, 0}, 1.0f, WHITE);

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

    // Draw culled geometry using GeometryRenderer (GPU frustum culling)
    renderManager.GetGeometryRenderer()->Draw(cameraController.camera);

    // Draw collision boxes for debugging
    if (showCollisionBoxes)
        collisionSystem.DrawDebug(false);

    // Draw NPCs
    for (auto &npc : npcs)
    {
        npc.Draw();
    }

    // Draw water (semi-transparent, after opaque geometry)
    renderManager.GetWaterRenderer()->Draw();

    // Draw grass LAST (after all opaque geometry, for proper depth testing)
    if (showGrass)
        renderManager.GetGrassRenderer()->Draw(cameraController.camera);

    // Draw speech bubble backgrounds (must be in 3D mode)
    speechBubbleManager.DrawBackgrounds(cameraController.camera);
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

    // Draw NPC speech bubble TEXT (must be in 2D/UI mode)
    speechBubbleManager.DrawText(cameraController.camera);
}

void Game::Draw2DUI()
{
    int yPos = UI_MARGIN;

    // Controls help
    DrawText("WASD: Move | Mouse: Look | E: Talk to NPC | TAB: Toggle Cursor | DELETE: Exit", UI_MARGIN, yPos, UI_TEXT_SIZE, DARKGRAY);
    yPos += UI_LINE_SPACING;

    // NPC interaction hint
    for (const auto &npc : npcs)
    {
        if (npc.IsInteractable())
        {
            DrawText(TextFormat("[E] Talk to %s", npc.GetName().c_str()), UI_MARGIN, yPos, UI_TEXT_SIZE, YELLOW);
            yPos += UI_LINE_SPACING;
            break; // Only show one hint at a time
        }
    }

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
    // FPS: always show in top-right corner using current window width
    DrawFPS(GetScreenWidth() - 100, UI_MARGIN);

    // Geometry culling stats (below FPS)
    GeometryRenderer *geoRenderer = renderManager.GetGeometryRenderer();
    int visibleGeometry = geoRenderer->GetVisibleCount();
    int totalGeometry = geoRenderer->GetTotalCount();
    DrawText(TextFormat("Geometry: %d/%d", visibleGeometry, totalGeometry),
             GetScreenWidth() - 200, UI_MARGIN + 25, UI_SMALL_TEXT_SIZE,
             geoRenderer->IsGPUCullingEnabled() ? LIME : ORANGE);

    // Grass culling stats (below geometry stats)
    GrassRenderer *grassRenderer = renderManager.GetGrassRenderer();
    int visibleGrass = grassRenderer->GetVisibleCount();
    int totalGrass = grassRenderer->GetTotalCount();
    DrawText(TextFormat("Grass: %d/%d", visibleGrass, totalGrass),
             GetScreenWidth() - 200, UI_MARGIN + 45, UI_SMALL_TEXT_SIZE, SKYBLUE);

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

    // Debug menus
    debugMenu.Draw();
    postProcessingMenu.Draw();
}

void Game::Shutdown()
{
    // Shutdown render manager - handles all rendering subsystems
    renderManager.Shutdown();

    if (player.modelLoaded)
        UnloadModel(player.model);

    UnloadModel(pbrTestSphere);
    UnloadModel(pbrRedCube);
    UnloadModel(pbrBlueTower);
    UnloadModel(pbrYellowSphere);
    UnloadModel(pbrOrangeSphere);
    UnloadModel(pbrCapsule);
    UnloadModel(pbrCylinder);
    UnloadModel(pbrGroundPlane);
    UnloadModel(pbrRamp);
    UnloadModel(pbrSteepRamp);

    CloseWindow();
}

bool Game::ShouldClose()
{
    return WindowShouldClose() || IsKeyPressed(KEY_DELETE) || currentState == GameState::QUIT;
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
