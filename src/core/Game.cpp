#include "Game.h"
#include "../world/LuaScene.h"
#include "../graphics/BillboardText.h"
#include "ui/rmlui/GameEventListener.h"
#include "rlgl.h"
#include <memory>

void Game::Init()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Zelda-like 3D Game Structure");
    SetTargetFPS(TARGET_FPS);
    SetExitKey(KEY_NULL);

    // Start with cursor enabled for main menu
    EnableCursor();

    // Initialize menus first
    SetupMenus();

    // Initialize RmlUi
    rmlReady = RaylibRmlUi::Initialize(GetScreenWidth(), GetScreenHeight());
    if (rmlReady)
    {
        // Load fonts before loading RML documents
        RaylibRmlUi::LoadFontFace("assets/ui/rml/fonts/LatoLatin-Regular.ttf", "Lato", false);
        RaylibRmlUi::LoadFontFace("assets/ui/rml/fonts/LatoLatin-Bold.ttf", "Lato Bold", false);
        RaylibRmlUi::LoadFontFace("assets/ui/rml/fonts/UbuntuMono-Regular.ttf", "Ubuntu Mono", false);

        // Load main menu
        RaylibRmlUi::LoadRml("assets/ui/rml/mainmenu.rml", "mainmenu", false);
        rmlMainMenu = RaylibRmlUi::GetPage("mainmenu");
        if (rmlMainMenu)
        {
            // Setup main menu button handlers
            auto btnPlay = rmlMainMenu->GetElementById("btn-play");
            auto btnSettings = rmlMainMenu->GetElementById("btn-settings");
            auto btnQuit = rmlMainMenu->GetElementById("btn-quit");

            if (btnPlay)
                btnPlay->AddEventListener(Rml::EventId::Click, new GameEventListener([this](Rml::Event &)
                                                                                     { ChangeState(GameState::PLAYING); }));
            if (btnSettings)
                btnSettings->AddEventListener(Rml::EventId::Click, new GameEventListener([this](Rml::Event &)
                                                                                         { ChangeState(GameState::SETTINGS); }));
            if (btnQuit)
                btnQuit->AddEventListener(Rml::EventId::Click, new GameEventListener([this](Rml::Event &)
                                                                                     { ChangeState(GameState::QUIT); }));
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "RmlUi failed to initialize; UI will be disabled");
    }

    // Initialize rendering system BEFORE creating scenes
    renderManager.Init(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Setup base renderer and lights
    SetupRenderer();

    // Register and load scene from Lua file
    LuaScene *testScene = new LuaScene("assets/scenes/test_scene.lua");
    sceneManager.RegisterScene("TestScene", testScene);
    sceneManager.LoadScene("TestScene");

    // Debug menu will be initialized after scene setup, once models exist

    // Start in main menu with RmlUI
    currentState = GameState::MAIN_MENU;
    EnableCursor();
    if (rmlMainMenu)
        rmlMainMenu->Show();
}

void Game::SetupParticles(const LevelData &level)
{
    ParticleSystem *ps = renderManager.GetParticleSystem();
    if (!ps)
        return;

    for (const auto &pData : level.particles)
    {
        EmitterConfig config;
        config.position = pData.position;
        config.offset = pData.offset;
        config.velocity = pData.velocity;
        config.velocityRandom = pData.velocityRandom;
        config.acceleration = pData.acceleration;
        config.colorStart = pData.colorStart;
        config.colorEnd = pData.colorEnd;
        config.sizeStart = pData.sizeStart;
        config.sizeEnd = pData.sizeEnd;
        config.sizeRandom = pData.sizeRandom;
        config.lifeMin = pData.lifeMin;
        config.lifeMax = pData.lifeMax;
        config.emissionRate = pData.emissionRate;
        config.maxParticles = pData.maxParticles;

        // Map blend mode string to enum
        config.blendMode = ParticleBlendMode::ALPHA;
        if (pData.blendMode == "add")
            config.blendMode = ParticleBlendMode::ADD;
        else if (pData.blendMode == "mul")
            config.blendMode = ParticleBlendMode::MULTIPLY;
        else if (pData.blendMode == "sub")
            config.blendMode = ParticleBlendMode::SUBTRACT;

        // Load texture priority: Path > Name > Default
        if (!pData.texturePath.empty())
        {
            config.texture = LoadTexture(pData.texturePath.c_str());
        }
        else if (!pData.textureName.empty())
        {
            config.texture = ps->GetTexture(pData.textureName);
        }
        else
        {
            config.texture = {0}; // Use default (which ps->CreateEmitter handles by calling GetTexture("soft_circle") if 0 or similar logic, but let's be safe)
            // CreateEmitter currently checks if texture.id > 0, else uses GetTextureDefault().
            // But wait, I changed GetTextureDefault usage.
            // Let's explicitly set it.
            config.texture = ps->GetTexture("soft_circle");
        }

        ps->CreateEmitter(config);
    }

    TraceLog(LOG_INFO, "Game: Particle system setup complete with %d emitters", (int)level.particles.size());
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

void Game::Update()
{
    const float deltaTime = GetFrameTime();

    ApplyDisplayModeIfChanged();
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
        break;
    }

    if (rmlReady)
        RaylibRmlUi::Update();
}

void Game::UpdateMainMenu()
{
    // RmlUI handles menu rendering automatically
    if (rmlMainMenu && !rmlMainMenu->IsVisible())
        rmlMainMenu->Show();
}

void Game::UpdatePlaying()
{
    const float deltaTime = GetFrameTime();

    // Hide main menu when playing
    if (rmlMainMenu && rmlMainMenu->IsVisible())
        rmlMainMenu->Hide();

    // Handle pause (ESC key)
    if (IsKeyPressed(KEY_ESCAPE))
    {
        ChangeState(GameState::PAUSED);
        return;
    }

    // Initialize scene on first update
    if (!sceneInitialized)
    {
        InitializeScene();
        sceneInitialized = true;
        TraceLog(LOG_INFO, "Game: First frame initialization complete");
        return; // Skip rest of update on first frame to let rendering catch up
    }

    // Update debug menu
    debugMenu.Update();
    postProcessingMenu.Update();

    // Apply cull margins from debug menu
    {
        GeometryRenderer *geoRenderer = renderManager.GetGeometryRenderer();
        if (geoRenderer)
            geoRenderer->SetCullingRadiusMultiplier(geometryCullMargin);
        GrassRenderer *grassRenderer = renderManager.GetGrassRenderer();
        if (grassRenderer)
            grassRenderer->SetCullingRadiusMultiplier(grassCullMargin);
    }

    // Apply player model selection from debug menu with safe clamping
    {
        int modelCount = customModel.getModelCount();
        if (modelCount > 0)
        {
            if (currentModelIndex < 0)
                currentModelIndex = 0;
            if (currentModelIndex >= modelCount)
                currentModelIndex = currentModelIndex % modelCount;
            if (currentModelIndex != previousModelIndex)
            {
                customModel.loadPlayerModel(player, currentModelIndex);
                previousModelIndex = currentModelIndex;
            }
        }
    }

    // Handle input
    HandleInput(deltaTime);

    // Update camera and rendering subsystems
    cameraController.Update(deltaTime);
    renderManager.UpdateCamera(cameraController.camera);
    renderManager.UpdateGrass(deltaTime, cameraController.camera);
    renderManager.UpdateGeometry(deltaTime, cameraController.camera);
    renderManager.UpdateWater(deltaTime, cameraController.camera);
    renderManager.UpdateParticles(deltaTime, cameraController.camera.position);
    renderManager.UpdateSkybox(deltaTime);

    // Update player
    UpdatePlayer(deltaTime);

    // Update NPCs
    UpdateNPCs(deltaTime);
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

void Game::UpdateSettings()
{
    settingsMenu.Update();
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

    if (rmlReady)
        RaylibRmlUi::SetViewport(newWidth, newHeight);

    // Rebuild menus so layout matches the new window size
    SetupMenus();

    if (currentState == GameState::SETTINGS)
    {
        settingsMenu.Init(newWidth, newHeight, &fullscreenMode, [this]()
                          { ChangeState(settingsReturnState); });
    }
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
        DrawPlaying(); // Scene handles its own rendering
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
    if (rmlReady)
        RaylibRmlUi::Draw();
}

void Game::DrawPlaying()
{
    if (!sceneInitialized)
    {
        ClearBackground(DARKGRAY);
        DrawText("Loading scene...", 10, 10, 20, WHITE);
        return;
    }

    // Check if scene is still loaded
    Scene *scene = sceneManager.GetCurrentScene();
    if (!scene)
    {
        ClearBackground(RED);
        DrawText("ERROR: Scene not loaded!", 10, 10, 20, WHITE);
        return;
    }

    // Use RenderManager for all rendering
    renderManager.BeginFrame();

    // Render 3D scene using callback with BeginMode3D inside
    renderManager.RenderScene([this]()
                              {
        BeginMode3D(cameraController.camera);
        DrawScene();
        EndMode3D(); }, cameraController.camera);

    renderManager.EndFrame();

    // Draw 2D UI on top
    DrawUI();
}

void Game::DrawPaused()
{
    pauseMenu.Draw();
}

void Game::DrawSettings()
{
    settingsMenu.Draw();
}

void Game::Shutdown()
{
    ShutdownScene();

    // Scene manager automatically unloads scenes on destruction

    // Shutdown render manager - handles all rendering subsystems
    renderManager.Shutdown();

    if (rmlReady)
        RaylibRmlUi::DeInitialize();

    CloseWindow();
}

bool Game::ShouldClose()
{
    return WindowShouldClose() || IsKeyPressed(KEY_DELETE) || currentState == GameState::QUIT;
}

// ===== SCENE INITIALIZATION =====

void Game::InitializeScene()
{
    Scene *scene = sceneManager.GetCurrentScene();
    if (!scene)
        return;

    LevelData &level = scene->GetLevelData();

    TraceLog(LOG_INFO, "Game: Initializing scene '%s'...", level.name.c_str());

    SetupCamera(level);
    SetupPlayer(level);
    SetupModels(level);
    SetupLights(level);
    SetupSkybox(level);
    SetupGrass(level);
    SetupWater(level);
    SetupParticles(level);
    SetupCollisions(level);

    // Now that models and systems are ready, set up the debug menu safely
    SetupDebugMenu();

    TraceLog(LOG_INFO, "Game: Scene initialization complete");
}

void Game::SetupRenderer()
{
    // Setup sun direction
    Vector3 sunDirection = {0.3f, 0.5f, 0.8f};
    renderManager.SetSunDirection(sunDirection);

    // Create lights (used by the PBR renderer) - matching old code
    LightRenderer *lightRenderer = renderManager.GetLightRenderer();
    lightRenderer->CreateDirectionalLight(sunDirection, {1.0f, 0.95f, 0.8f, 1.0f}, 2.0f);
    lightRenderer->CreatePointLight({-5.0f, 4.0f, -5.0f}, {1.0f, 0.9f, 0.8f, 1.0f}, 12.0f, 15.0f);
    lightRenderer->CreatePointLight({5.0f, 4.0f, 5.0f}, {0.8f, 0.9f, 1.0f, 1.0f}, 12.0f, 15.0f);
    lightRenderer->CreatePointLight({0.0f, 6.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 15.0f, 20.0f);

    TraceLog(LOG_INFO, "Game: Base renderer initialized with lights");
}

void Game::ShutdownScene()
{
    if (!sceneInitialized)
        return;

    TraceLog(LOG_INFO, "Game: Shutting down scene...");

    // Unload player model
    if (player.modelLoaded)
        UnloadModel(player.model);

    // Unload all scene models
    for (auto &pair : sceneModels)
    {
        UnloadModel(pair.second);
    }
    sceneModels.clear();
    modelIDs.clear();

    // Clear world entities
    world.Clear();

    sceneInitialized = false;

    TraceLog(LOG_INFO, "Game: Scene shutdown complete");
}

void Game::SetupDebugMenu()
{
    debugMenu.AddBool("Show Grid", &showGrid);
    debugMenu.AddBool("Show FPS", &showFPS);
    debugMenu.AddBool("Show Collision Boxes", &showCollisionBoxes);
    debugMenu.AddBool("Show Grass", &showGrass);
    debugMenu.AddFloat("Jump Strength", &player.jumpStrength, 1.0f, 20.0f, 0.1f);
    debugMenu.AddFloat("Gravity", &player.gravity, -50.0f, -5.0f, 0.1f);
    debugMenu.AddFloat("Sprint Multiplier", &player.sprintMultiplier, 1.0f, 5.0f, 0.1f);
    debugMenu.AddFloat("Collision Radius", &player.collisionRadius, 0.1f, 2.0f, 0.05f);
    debugMenu.AddFloat("Collision Height", &player.collisionHeight, 0.5f, 3.0f, 0.1f);
    debugMenu.AddFloat("Eye Height", &player.eyeHeight, 0.5f, 2.5f, 0.1f);

    // Culling margins
    debugMenu.AddFloat("Geometry Cull Margin", &geometryCullMargin, 1.0f, 2.0f, 0.05f);
    debugMenu.AddFloat("Grass Cull Margin", &grassCullMargin, 1.0f, 2.0f, 0.05f);

    std::vector<std::string> modelNames;
    modelNames.reserve(customModel.getModelCount());
    for (int i = 0; i < customModel.getModelCount(); i++)
        modelNames.push_back(customModel.getModelName(i));
    debugMenu.AddString("Player Model", &currentModelIndex, modelNames);

    debugMenu.AddFloat("Camera FOV", &cameraController.camera.fovy, 20.0f, 120.0f, 1.0f);

    TraceLog(LOG_INFO, "Game: Debug menu initialized");
}

void Game::SetupCamera(const LevelData &level)
{
    cameraController.Initialize(
        level.camera.startPosition,
        level.camera.startTarget,
        level.camera.startFOV);

    cameraController.SetMode(CAMERA_MODE_FOLLOW);
    cameraController.SetFollowTarget(&player.position);
    cameraController.SetFollowDistance(level.camera.followDistance);
    cameraController.SetFollowHeight(level.camera.followHeight);
    cameraController.SetSmoothness(level.camera.smoothness);
}

void Game::SetupPlayer(const LevelData &level)
{
    player.position = level.playerStartPosition;

    // Load player model
    customModel.addModel("Rat", "assets/models/rat.obj", "assets/textures/rat.png", {0.04f, 0.04f, 0.04f}, {0.0f, 0.0f, 0.0f});
    customModel.addModel("Miku", "assets/models/miku/scene.gltf", "", {1.8f, 1.8f, 1.8f}, {90.0f, 0.0f, 0.0f});
    customModel.loadPlayerModel(player, currentModelIndex);
}

Model Game::CreateModelFromType(const std::string &modelType)
{
    // Parse model type string
    if (modelType.find("plane") == 0)
    {
        return LoadModelFromMesh(GenMeshPlane(32.0f, 32.0f, 10, 10));
    }
    else if (modelType.find("sphere") == 0)
    {
        float radius = 1.0f;
        sscanf(modelType.c_str(), "sphere_%f", &radius);
        return LoadModelFromMesh(GenMeshSphere(radius, 64, 64));
    }
    else if (modelType.find("cube") == 0)
    {
        float x = 1.0f, y = 1.0f, z = 1.0f;
        if (modelType.find("cube_") == 0)
        {
            sscanf(modelType.c_str(), "cube_%fx%fx%f", &x, &y, &z);
        }
        else
        {
            float size = 1.0f;
            sscanf(modelType.c_str(), "cube_%f", &size);
            x = y = z = size;
        }
        return LoadModelFromMesh(GenMeshCube(x, y, z));
    }
    else if (modelType.find("cylinder") == 0)
    {
        float radius = 0.5f, height = 1.0f;
        sscanf(modelType.c_str(), "cylinder_%fx%f", &radius, &height);
        return LoadModelFromMesh(GenMeshCylinder(radius, height, 16));
    }

    // Default: cube
    return LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
}

void Game::SetupModels(const LevelData &level)
{
    GeometryRenderer *geoRenderer = renderManager.GetGeometryRenderer();
    std::vector<RenderManager::ModelMaterial> modelsToShader;

    for (const auto &objData : level.objects)
    {
        // Create model if not already created
        if (sceneModels.find(objData.modelType) == sceneModels.end())
        {
            sceneModels[objData.modelType] = CreateModelFromType(objData.modelType);
        }

        Model *model = &sceneModels[objData.modelType];

        // Register with geometry renderer if not already registered
        if (modelIDs.find(objData.name) == modelIDs.end())
        {
            int modelID = geoRenderer->RegisterModel(objData.name, model);
            modelIDs[objData.name] = modelID;
        }
        // Add instance for culling system (use rotation and position from Lua; models encode size)
        {
            int modelID = modelIDs[objData.name];
            geoRenderer->AddInstance(modelID, objData.position, 1.0f, objData.rotation);
        }

        // Add to shader application list
        Vector4 albedoVec = ColorNormalize(objData.albedo);
        modelsToShader.push_back({model, albedoVec, objData.metallic, objData.roughness});

        // Create world entity
        using namespace World;
        Entity e = world.CreateEntity();
        world.AddMetadata(e, objData.name, true);
        world.AddTransform(e, objData.position, objData.rotation, objData.scale);
        world.AddRender(e, model, modelIDs[objData.name], objData.albedo, objData.metallic, objData.roughness);
    }

    // Apply PBR shaders to all models
    renderManager.ApplyShaders(modelsToShader);
}

void Game::SetupLights(const LevelData &level)
{
    LightRenderer *lightRenderer = renderManager.GetLightRenderer();

    // Clear existing lights (except we want to keep the sun from SetupRenderer)
    // So we'll just add the scene lights on top

    for (const auto &lightData : level.lights)
    {
        if (lightData.type == 0) // Directional
        {
            Vector4 colorVec = ColorNormalize(lightData.color);
            lightRenderer->CreateDirectionalLight(
                lightData.direction,
                colorVec,
                lightData.intensity);
        }
        else // Point
        {
            Vector4 colorVec = ColorNormalize(lightData.color);
            lightRenderer->CreatePointLight(
                lightData.position,
                colorVec,
                lightData.intensity,
                lightData.radius);
        }
    }
}

void Game::SetupSkybox(const LevelData &level)
{
    if (!level.skyboxTexture.empty())
    {
        renderManager.GetSkyboxRenderer()->Load("assets/shader/skybox.vs", "assets/shader/skybox.fs");
    }
}

void Game::SetupGrass(const LevelData &level)
{
    // Initialize grass renderer with fixed reasonable values like old code
    // 200000 grass blades over a 30x30 area (old code used these exact values)
    renderManager.InitializeGrass(200000, 30.0f);
    renderManager.ConfigureGrass({1.0f, 0.5f}, 0.5f, 2.0f);
}

void Game::SetupWater(const LevelData &level)
{
    // Initialize water renderer with fixed reasonable values like old code
    // 50x50 water plane at -0.5 height
    renderManager.InitializeWater(50.0f, 50.0f, -0.5f);
}

void Game::SetupCollisions(const LevelData &level)
{
    for (const auto &objData : level.objects)
    {
        if (objData.collisionType == "none")
            continue;

        using namespace World;

        // Find the entity by name
        for (Entity e = 0; e < world.GetEntityCount(); ++e)
        {
            if (!world.IsValid(e))
                continue;

            auto &allMetadata = world.GetMetadata();
            int metaIdx = world.GetMetadataIndex(e);
            if (metaIdx < 0)
                continue;

            const std::string &name = allMetadata.names[metaIdx];
            if (name == objData.name)
            {
                CollisionShape collShape = COLLISION_BOX;
                if (objData.collisionType == "sphere")
                    collShape = COLLISION_SPHERE;
                else if (objData.collisionType == "cylinder")
                    collShape = COLLISION_CYLINDER;
                else if (objData.collisionType == "capsule")
                    collShape = COLLISION_CAPSULE;

                world.AddCollision(e, collShape, objData.collisionSize,
                                   objData.collisionRadius, objData.collisionHeight,
                                   objData.rotation, BLUE);

                // Add to collision system (translate + rotate boxes)
                if (collShape == COLLISION_BOX)
                    collisionSystem.AddBox(objData.position, objData.collisionSize, objData.name, BLUE, objData.rotation);
                else if (collShape == COLLISION_SPHERE)
                    collisionSystem.AddSphere(objData.position, objData.collisionRadius, objData.name);
                else if (collShape == COLLISION_CAPSULE)
                    collisionSystem.AddCapsule(objData.position, objData.collisionRadius, objData.collisionHeight, objData.name);
                else if (collShape == COLLISION_CYLINDER)
                    collisionSystem.AddCylinder(objData.position, objData.collisionRadius, objData.collisionHeight, objData.name);
                break;
            }
        }
    }
}

// ===== GAME LOOP =====

void Game::UpdatePlayer(float deltaTime)
{
    if (!cameraController.IsCutscenePlaying() && cameraController.mode != CAMERA_MODE_FIXED)
    {
        player.UpdatePlayerMovementWithCollision(cameraController.camera, &collisionSystem);
    }
}

void Game::UpdateNPCs(float deltaTime)
{
    Scene *scene = sceneManager.GetCurrentScene();
    if (!scene)
        return;

    std::vector<NPC> &npcs = scene->GetNPCs();

    for (auto &npc : npcs)
    {
        npc.Update(player.position);
    }
}

void Game::HandleNPCInteraction()
{
    Scene *scene = sceneManager.GetCurrentScene();
    if (!scene)
        return;

    std::vector<NPC> &npcs = scene->GetNPCs();

    if (IsKeyPressed(KEY_E))
    {
        for (auto &npc : npcs)
        {
            if (npc.IsInteractable())
            {
                std::string dialogue = npc.GetNextDialogue();
                TraceLog(LOG_INFO, "NPC Dialogue: %s", dialogue.c_str());
                break;
            }
        }
    }
}

void Game::HandleInput(float deltaTime)
{
    HandleNPCInteraction();

    // Toggle cursor
    if (IsKeyPressed(KEY_TAB))
    {
        if (IsCursorHidden())
            EnableCursor();
        else
            DisableCursor();
    }

    if (rmlReady && IsKeyPressed(KEY_F8))
    {
        RaylibRmlUi::ToggleDebugger();
    }
}

void Game::DrawScene()
{
    // Draw skybox
    renderManager.GetSkyboxRenderer()->Draw(cameraController.camera);

    // Draw culled geometry via GeometryRenderer
    renderManager.GetGeometryRenderer()->Draw(cameraController.camera);

    // Draw player with proper rotation transform
    customModel.drawPlayerModel(player);

    // Draw colored spheres at point light positions from scene data
    {
        Scene *s = sceneManager.GetCurrentScene();
        if (s)
        {
            const LevelData &lvl = s->GetLevelData();
            for (const auto &light : lvl.lights)
            {
                if (light.type == 1) // point light
                {
                    DrawSphere(light.position, 0.3f, light.color);
                }
            }
        }
    }

    // Draw collision boxes
    if (showCollisionBoxes)
        collisionSystem.DrawDebug(false);

    // Draw NPCs
    Scene *scene = sceneManager.GetCurrentScene();
    if (scene)
    {
        for (auto &npc : scene->GetNPCs())
        {
            npc.Draw();
        }
    }

    // Draw water
    renderManager.GetWaterRenderer()->Draw();

    // Draw grass
    if (showGrass)
        renderManager.GetGrassRenderer()->Draw(cameraController.camera);

    // Draw particles
    renderManager.GetParticleSystem()->Draw(cameraController.camera);

    // Draw grid for debugging
    if (showGrid)
        DrawGrid(20, 1.0f);
}

void Game::DrawUI()
{
    const int UI_MARGIN = 10;
    const int UI_TEXT_SIZE = 20;
    const int UI_SMALL_TEXT_SIZE = 16;
    const int UI_LINE_SPACING = 30;

    int yPos = UI_MARGIN;

    // Controls help
    DrawText("WASD: Move | Mouse: Look | E: Talk | TAB: Cursor | ESC: Pause | DELETE: Exit",
             UI_MARGIN, yPos, UI_TEXT_SIZE, DARKGRAY);
    yPos += UI_LINE_SPACING;

    // NPC interaction hint
    Scene *scene = sceneManager.GetCurrentScene();
    if (scene)
    {
        for (const auto &npc : scene->GetNPCs())
        {
            if (npc.IsInteractable())
            {
                DrawText(TextFormat("[E] Talk to %s", npc.GetName().c_str()),
                         UI_MARGIN, yPos, UI_TEXT_SIZE, YELLOW);
                yPos += UI_LINE_SPACING;
                break;
            }
        }
    }

    // Camera mode
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
    DrawText(TextFormat("Camera: %s", modeText), UI_MARGIN, yPos, UI_TEXT_SIZE, GREEN);

    // FPS
    DrawFPS(GetScreenWidth() - 100, UI_MARGIN);

    // Geometry culling stats
    GeometryRenderer *geoRenderer = renderManager.GetGeometryRenderer();
    int visibleGeometry = geoRenderer->GetVisibleCount();
    int totalGeometry = geoRenderer->GetTotalCount();
    DrawText(TextFormat("Geometry: %d/%d", visibleGeometry, totalGeometry),
             GetScreenWidth() - 200, UI_MARGIN + 25, UI_SMALL_TEXT_SIZE,
             geoRenderer->IsGPUCullingEnabled() ? LIME : ORANGE);

    // Grass culling stats
    GrassRenderer *grassRenderer = renderManager.GetGrassRenderer();
    int visibleGrass = grassRenderer->GetVisibleCount();
    int totalGrass = grassRenderer->GetTotalCount();
    DrawText(TextFormat("Grass: %d/%d", visibleGrass, totalGrass),
             GetScreenWidth() - 200, UI_MARGIN + 45, UI_SMALL_TEXT_SIZE, SKYBLUE);

    // Draw debug menus
    debugMenu.Draw();
    postProcessingMenu.Draw();

    if (rmlReady)
    {
        RaylibRmlUi::Draw();
    }
}
