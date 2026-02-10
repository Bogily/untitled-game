#include "Game.h"
#include "../world/LuaScene.h"
#include "../graphics/BillboardText.h"
#include "../utils/ShaderUtil.h"
#include "ui/rmlui/GameEventListener.h"
#include "rlgl.h"
#include <glad/glad.h>
#include <memory>

void Game::Init()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Zelda-like 3D Game Structure");
    SetTargetFPS(TARGET_FPS);
    SetExitKey(KEY_NULL);

    // Initialize GLAD for OpenGL function loading
    if (!gladLoadGL())
    {
        TraceLog(LOG_ERROR, "Failed to initialize GLAD!");
        return;
    }
    TraceLog(LOG_INFO, "GLAD initialized successfully");

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
        RaylibRmlUi::LoadRml("assets/ui/rml/mainmenu.rhtml", "mainmenu", false);
        rmlMainMenu = RaylibRmlUi::GetPage("mainmenu");
        TraceLog(LOG_INFO, "RmlUi: mainmenu page loaded: %s", rmlMainMenu ? "YES" : "NO");
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
            TraceLog(LOG_INFO, "RmlUi: Event listeners set up successfully");
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "RmlUi failed to initialize; UI will be disabled");
    }

    TraceLog(LOG_INFO, "Initializing rendering system...");
    // Initialize rendering system BEFORE creating scenes
    renderManager.Init(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Load main menu background shader
    TraceLog(LOG_INFO, "Loading main menu background shader...");
    mainMenuBackgroundShader = LoadShader("assets/shader/fullscreen.vs", "assets/shader/fire.fs");
    if (mainMenuBackgroundShader.id > 0)
    {
        mainMenuShaderReady = true;
        TraceLog(LOG_INFO, "Main menu fire shader loaded successfully");
    }
    else
    {
        TraceLog(LOG_WARNING, "Failed to load main menu fire shader");
    }

    TraceLog(LOG_INFO, "Loading scene from Lua...");
    // Register and load scene from Lua file
    LuaScene *testScene = new LuaScene("assets/scenes/test_scene.lua");
    sceneManager.RegisterScene("TestScene", testScene);
    sceneManager.LoadScene("TestScene");

    TraceLog(LOG_INFO, "Scene loaded successfully");
    // Debug menu will be initialized after scene setup, once models exist

    // Start in main menu with RmlUI
    currentState = GameState::MAIN_MENU;
    EnableCursor();
    if (rmlMainMenu)
        rmlMainMenu->Show();

    TraceLog(LOG_INFO, "Game initialization complete");
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
    if (rmlReady && rmlMainMenu && !rmlMainMenu->IsVisible())
        rmlMainMenu->Show();
}

void Game::UpdatePlaying()
{
    const float deltaTime = GetFrameTime();

    // Hide main menu when playing
    if (rmlReady && rmlMainMenu && rmlMainMenu->IsVisible())
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

    // Update debug menu (modifies member variables via pointers)
    debugMenu.Update();
    postProcessingMenu.Update();

    // Apply player model selection from debug menu
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

    // Apply frame settings to render manager (from member variables modified by menus)
    renderFrameSettings.showDebugGrid = showGrid;
    renderFrameSettings.showGrass = showGrass;
    renderFrameSettings.geometryCullMargin = geometryCullMargin;
    renderFrameSettings.grassCullMargin = grassCullMargin;
    renderFrameSettings.grayscaleEnabled = enableGrayscale;
    renderFrameSettings.depthDebugEnabled = enableDepthDebug;

    // Apply color grading settings
    PostProcessingRenderer *postProc = renderManager.GetPostProcessingRenderer();
    if (postProc)
    {
        postProc->SetColorGradingPreset(colorGradingPreset);
        postProc->SetColorGradingIntensity(colorGradingIntensity);

        // Apply contact shadows settings
        postProc->SetContactShadowsEnabled(enableContactShadows);
        postProc->SetContactShadowParams(contactShadowDistance, contactShadowSteps,
                                         contactShadowThickness, contactShadowIntensity);

        // Apply SSAO settings
        postProc->SetSSAOEnabled(enableSSAO);
        postProc->SetSSAOParams(ssaoNumSamples, ssaoRadius, ssaoBias, ssaoIntensity, ssaoContrast);
    }

    // Map MSAA index to MSAALevel
    switch (msaaLevelIndex)
    {
    case 0:
        renderFrameSettings.msaaLevel = RenderManager::MSAALevel::MSAA_NONE;
        break;
    case 1:
        renderFrameSettings.msaaLevel = RenderManager::MSAALevel::MSAA_4X;
        break;
    case 2:
        renderFrameSettings.msaaLevel = RenderManager::MSAALevel::MSAA_8X;
        break;
    case 3:
        renderFrameSettings.msaaLevel = RenderManager::MSAALevel::MSAA_16X;
        break;
    default:
        renderFrameSettings.msaaLevel = RenderManager::MSAALevel::MSAA_NONE;
    }

    renderManager.ApplyFrameSettings(renderFrameSettings);

    // Unified rendering system update - handles camera and all subsystems
    renderManager.UpdateAllSystems(deltaTime);

    // Set follow target for camera
    CameraController *camCtrl = renderManager.GetCameraController();
    camCtrl->SetFollowTarget(&player.GetTransform().position);

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
        ClearBackground(BLACK);
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
    ClearBackground(BLACK);

    if (mainMenuShaderReady && flameIntensity > 0.0f)
    {
        // Set shader uniforms
        ShaderUtil util(mainMenuBackgroundShader);
        util.SetFloat("uTime", (float)GetTime());
        util.SetVec3("uFlameColor", flameColor);
        util.SetFloat("uIntensity", flameIntensity);

        // Draw fullscreen quad with shader
        BeginShaderMode(mainMenuBackgroundShader);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
        EndShaderMode();
    }

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

    // Unified rendering pipeline - simpler and more efficient
    renderManager.BeginFrame();

    // Render 3D scene with all subsystems
    Camera3D renderCamera = renderManager.GetCameraController()->camera;
    renderManager.RenderScene([this, renderCamera]()
                              {
        BeginMode3D(renderCamera);
        
        // Draw all 3D elements (order matters for proper layering)
        renderManager.GetSkyboxRenderer()->Draw(renderCamera);
        renderManager.GetGeometryRenderer()->Draw(renderCamera);
        
        // Draw player with proper rotation transform
        customModel.drawPlayerModel(player);
        
        // Draw NPCs
        Scene *s = sceneManager.GetCurrentScene();
        if (s)
        {
            for (auto &npc : s->GetNPCs())
            {
                npc.Draw();
            }
        }
        
        // Draw water
        renderManager.GetWaterRenderer()->Draw();
        
        // Draw grass if enabled
        if (showGrass)
            renderManager.GetGrassRenderer()->Draw(renderCamera);
        
        // Draw particles
        renderManager.GetParticleSystem()->Draw(renderCamera);
        
        // Draw debug grid if enabled
        if (showGrid)
            DrawGrid(20, 1.0f);
        
        EndMode3D(); }, renderCamera);

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

    if (mainMenuShaderReady)
        UnloadShader(mainMenuBackgroundShader);

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

    // Game-logic specific setup
    SetupPlayer(level);
    SetupModels(level);

    // All rendering setup delegated to RenderManager
    renderManager.SetupFromLevelData(level);

    // Setup camera controller's follow target
    CameraController *camCtrl = renderManager.GetCameraController();
    camCtrl->SetFollowTarget(&player.GetTransform().position);
    camCtrl->SetMode(CAMERA_MODE_FOLLOW);

    // Now that models and systems are ready, set up the menus safely
    SetupDebugMenu();
    SetupPostProcessingMenu();

    TraceLog(LOG_INFO, "Game: Scene initialization complete");
}

void Game::ShutdownScene()
{
    if (!sceneInitialized)
        return;

    TraceLog(LOG_INFO, "Game: Shutting down scene...");

    // Unload player model
    if (player.GetRender().modelLoaded)
        UnloadModel(player.GetRender().model);

    // Unload all scene models
    for (auto &pair : sceneModels)
    {
        UnloadModel(pair.second);
    }
    sceneModels.clear();
    modelIDs.clear();

    sceneInitialized = false;

    TraceLog(LOG_INFO, "Game: Scene shutdown complete");
}

void Game::SetupDebugMenu()
{
    debugMenu.AddBool("Show Grid", &showGrid);
    debugMenu.AddBool("Show FPS", &showFPS);
    debugMenu.AddBool("Show Grass", &showGrass);
    debugMenu.AddFloat("Sprint Multiplier", &player.sprintMultiplier, 1.0f, 5.0f, 0.1f);
    debugMenu.AddFloat("Eye Height", &player.eyeHeight, 0.5f, 2.5f, 0.1f);

    // Culling margins
    debugMenu.AddFloat("Geometry Cull Margin", &geometryCullMargin, 1.0f, 2.0f, 0.05f);
    debugMenu.AddFloat("Grass Cull Margin", &grassCullMargin, 1.0f, 2.0f, 0.05f);

    std::vector<std::string> modelNames;
    modelNames.reserve(customModel.getModelCount());
    for (int i = 0; i < customModel.getModelCount(); i++)
        modelNames.push_back(customModel.getModelName(i));
    debugMenu.AddString("Player Model", &currentModelIndex, modelNames);

    // Access camera from RenderManager
    CameraController *camCtrl = renderManager.GetCameraController();
    debugMenu.AddFloat("Camera FOV", &camCtrl->camera.fovy, 20.0f, 120.0f, 1.0f);

    TraceLog(LOG_INFO, "Game: Debug menu initialized");
}

void Game::SetupPostProcessingMenu()
{
    postProcessingMenu.AddBool("Grayscale", &enableGrayscale);
    postProcessingMenu.AddBool("Depth Debug", &enableDepthDebug);
    postProcessingMenu.AddString("MSAA Level", &msaaLevelIndex, {"None", "4X", "8X", "16X"});
    postProcessingMenu.AddString("Color Grading", &colorGradingPreset, {"None", "Warm", "Cool", "Cinematic", "Vintage", "Noir"});
    postProcessingMenu.AddFloat("Grading Intensity", &colorGradingIntensity, 0.0f, 1.0f, 0.05f);
    postProcessingMenu.AddBool("Contact Shadows", &enableContactShadows);
    postProcessingMenu.AddFloat("Shadow Distance", &contactShadowDistance, 0.01f, 0.5f, 0.02f);
    postProcessingMenu.AddInt("Shadow Steps", &contactShadowSteps, 4, 64, 4);
    postProcessingMenu.AddFloat("Shadow Thickness", &contactShadowThickness, 0.001f, 0.1f, 0.005f);
    postProcessingMenu.AddFloat("Shadow Intensity", &contactShadowIntensity, 0.0f, 1.0f, 0.05f);
    postProcessingMenu.AddBool("SSAO", &enableSSAO);
    postProcessingMenu.AddInt("AO Samples", &ssaoNumSamples, 4, 32, 2);
    postProcessingMenu.AddFloat("AO Radius", &ssaoRadius, 0.001f, 0.1f, 0.005f);
    postProcessingMenu.AddFloat("AO Bias", &ssaoBias, 0.001f, 0.01f, 0.001f);
    postProcessingMenu.AddFloat("AO Intensity", &ssaoIntensity, 0.0f, 2.0f, 0.05f);
    postProcessingMenu.AddFloat("AO Contrast", &ssaoContrast, 0.5f, 2.0f, 0.05f);

    TraceLog(LOG_INFO, "Game: Post-processing menu initialized");
}

void Game::SetupPlayer(const LevelData &level)
{
    player.GetTransform().position = level.playerStartPosition;

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
    }

    // Apply PBR shaders to all models
    renderManager.ApplyShaders(modelsToShader);
}

// ===== GAME LOOP =====

void Game::UpdateNPCs(float deltaTime)
{
    Scene *scene = sceneManager.GetCurrentScene();
    if (!scene)
        return;

    std::vector<NPC> &npcs = scene->GetNPCs();

    for (auto &npc : npcs)
    {
        npc.Update(player.GetTransform().position);
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

    // Camera mode - get from RenderManager
    CameraController *camCtrl = renderManager.GetCameraController();
    const char *modeText = "";
    switch (camCtrl->mode)
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
