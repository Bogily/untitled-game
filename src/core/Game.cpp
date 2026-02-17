#include "Game.h"
#include "../world/LuaScene.h"
#include "../utils/ShaderUtil.h"
#include "ui/rmlui/GameEventListener.h"
#include "rlgl.h"
#include <glad/glad.h>
#include <memory>
#include <filesystem>
#include <algorithm>
#include <unordered_set>

namespace
{
    float GetUniformScale(Vector3 scale)
    {
        return (scale.x + scale.y + scale.z) / 3.0f;
    }
}

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

    // Initialize GPU-accelerated SDF collision system
    collisionSystem.SetResolution(sdfResolution);
    collisionSystem.Init();

    DiscoverAndRegisterScenes();
    if (sceneManager.HasScene("test_scene"))
    {
        sceneManager.LoadScene("test_scene");
    }
    else if (!availableScenes.empty())
    {
        sceneManager.LoadScene(availableScenes.front());
    }

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

    // Apply camera mode selection from debug menu
    {
        CameraController *camCtrl = renderManager.GetCameraController();
        if (camCtrl)
        {
            // Clamp camera mode index
            if (cameraModeIndex < 0)
                cameraModeIndex = 0;
            if (cameraModeIndex > 3)
                cameraModeIndex = 3;

            // Map index to camera mode and apply
            CameraControllerMode modes[] = {CAMERA_MODE_FREE, CAMERA_MODE_FOLLOW, CAMERA_MODE_CUTSCENE, CAMERA_MODE_FIXED};
            camCtrl->SetMode(modes[cameraModeIndex]);

            // Apply free camera parameters (convert 0-1 sensitivity to actual value)
            camCtrl->SetFreeCameraSpeed(freeCameraSpeed);
            camCtrl->SetFreeCameraMouseSensitivity(freeCameraMouseSensitivity * 0.01f);
        }
    }

    // Handle input
    HandleInput(deltaTime);

    // Move player with WASD, apply gravity, and resolve SDF collisions
    UpdatePlayerMovement(deltaTime);

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

        Scene *s = sceneManager.GetCurrentScene();
        if (s)
        {
            for (const auto &objData : s->GetObjects())
            {
                if (objData.mobility != LevelData::ObjectData::Mobility::Dynamic)
                    continue;

                auto modelIt = sceneModels.find(objData.modelType);
                if (modelIt == sceneModels.end())
                    continue;

                rlPushMatrix();
                rlTranslatef(objData.position.x, objData.position.y, objData.position.z);
                rlRotatef(objData.rotation.x, 1.0f, 0.0f, 0.0f);
                rlRotatef(objData.rotation.y, 0.0f, 1.0f, 0.0f);
                rlRotatef(objData.rotation.z, 0.0f, 0.0f, 1.0f);
                rlScalef(objData.scale.x, objData.scale.y, objData.scale.z);
                DrawModel(modelIt->second, {0, 0, 0}, 1.0f, WHITE);
                rlPopMatrix();
            }
        }
        
        // Draw player with proper rotation transform
        customModel.drawPlayerModel(player);
        
        // Draw NPCs
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
        
        // Draw SDF collision debug visualisation
        if (showCollisionDebug && collisionSystem.IsReady())
        {
            collisionSystem.DrawDebugVolume(renderCamera, 1.15f);
            collisionSystem.DrawDebugBounds(GREEN);
        }
        
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

    // Shutdown SDF collision system (releases GPU resources)
    collisionSystem.Shutdown();

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

    // Prepare scene-bound render systems first (lights, skybox, grass/water, particles, camera)
    renderManager.SetupFromLevelData(level);

    // Register scene geometry after renderer scene reset so model pointers/instances stay valid
    SetupModels(*scene);

    // Build the GPU-accelerated SDF collision field from static scene geometry
    if (collisionSystem.IsEnabled())
    {
        Scene *s = sceneManager.GetCurrentScene();
        if (s)
        {
            collisionSystem.BuildSDF(s->GetObjects(), sceneModels);
            TraceLog(LOG_INFO, "Game: SDF collision field built (resolution=%d, voxel=%.4f)",
                     collisionSystem.GetResolution(), collisionSystem.GetVoxelSize());
        }
    }

    // Setup camera controller's follow target
    CameraController *camCtrl = renderManager.GetCameraController();
    camCtrl->SetFollowTarget(&player.GetTransform().position);
    camCtrl->SetMode(CAMERA_MODE_FOLLOW);

    // Reset player vertical velocity for the new scene
    playerVerticalVelocity = 0.0f;

    // Now that models and systems are ready, set up the menus safely
    // Clear and reinitialize debug menu to avoid duplicate entries
    debugMenu.Clear();
    SetupDebugMenu();
    postProcessingMenu.Clear();
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
    {
        UnloadModel(player.GetRender().model);
        player.GetRender().model = {0};
        player.GetRender().modelLoaded = false;
    }

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

void Game::DiscoverAndRegisterScenes()
{
    availableScenes.clear();

    const std::filesystem::path scenesPath("assets/scenes");
    if (!std::filesystem::exists(scenesPath) || !std::filesystem::is_directory(scenesPath))
    {
        TraceLog(LOG_WARNING, "Game: Scene directory not found: %s", scenesPath.string().c_str());
        return;
    }

    for (const auto &entry : std::filesystem::directory_iterator(scenesPath))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua")
            continue;

        const std::string sceneName = entry.path().stem().string();
        availableScenes.push_back(sceneName);

        if (!sceneManager.HasScene(sceneName))
        {
            sceneManager.RegisterScene(sceneName, new LuaScene(entry.path().string()));
        }
    }

    std::sort(availableScenes.begin(), availableScenes.end());
    TraceLog(LOG_INFO, "Game: Discovered %d scenes", (int)availableScenes.size());
}

bool Game::SwitchToScene(const std::string &sceneName)
{
    if (!sceneManager.HasScene(sceneName))
    {
        TraceLog(LOG_ERROR, "Game: Scene '%s' is not registered", sceneName.c_str());
        return false;
    }

    ShutdownScene();
    sceneManager.LoadScene(sceneName);

    if (!sceneManager.GetCurrentScene())
    {
        TraceLog(LOG_ERROR, "Game: Failed to activate scene '%s'", sceneName.c_str());
        return false;
    }

    InitializeScene();
    sceneInitialized = true;
    return true;
}

void Game::SetupDebugMenu()
{
    DiscoverAndRegisterScenes();

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

    // Camera mode selection
    debugMenu.AddString("Camera Mode", &cameraModeIndex, {"Free", "Follow", "Cutscene", "Fixed"});
    debugMenu.AddFloat("Free Cam Speed", &freeCameraSpeed, 1.0f, 50.0f, 1.0f);
    debugMenu.AddFloat("Free Cam Sens", &freeCameraMouseSensitivity, 0.1f, 1.0f, 0.05f);

    // SDF Collision controls
    debugMenu.AddBool("Enable Collision", &enableCollision);
    debugMenu.AddBool("Show Collision Debug", &showCollisionDebug);
    debugMenu.AddFloat("Collision Debug Y", &collisionDebugYLevel, -10.0f, 20.0f, 0.25f);
    debugMenu.AddFloat("Player Move Speed", &playerMoveSpeed, 1.0f, 20.0f, 0.5f);
    debugMenu.AddFloat("Player Collision Radius", &playerCollisionRadius, 0.1f, 2.0f, 0.05f);
    debugMenu.AddFloat("Player Gravity", &playerGravity, 0.0f, 40.0f, 1.0f);
    debugMenu.AddButton("Rebuild SDF", [this]()
                        {
        Scene *s = sceneManager.GetCurrentScene();
        if (s && collisionSystem.IsEnabled())
        {
            collisionSystem.BuildSDF(s->GetObjects(), sceneModels);
            TraceLog(LOG_INFO, "SDF collision field rebuilt from debug menu");
        } });

    // Camera effects
    debugMenu.AddFloat("Shake Intensity", &cameraShakeIntensity, 0.0f, 1.0f, 0.1f);
    debugMenu.AddFloat("Shake Duration", &cameraShakeDuration, 0.1f, 5.0f, 0.1f);

    // Button triggers
    debugMenu.AddButton("Apply Shake", [this, camCtrl]()
                        {
        if (camCtrl && cameraShakeIntensity > 0.0f && cameraShakeDuration > 0.0f)
        {
            camCtrl->ApplyShake(cameraShakeIntensity, cameraShakeDuration);
            TraceLog(LOG_INFO, "Camera shake applied (intensity=%.2f, duration=%.2f)", cameraShakeIntensity, cameraShakeDuration);
        } });

    debugMenu.AddButton("Play Example Cutscene", [this, camCtrl]()
                        {
        if (camCtrl)
        {
            // Define example cutscene waypoints
            std::vector<CameraWaypoint> cutsceneWaypoints = {
                // Start position: above and behind
                {{-15.0f, 12.0f, 15.0f}, {0.0f, 5.0f, 0.0f}, 3.0f, 45.0f},
                // Pan to the center sphere
                {{0.0f, 8.0f, 12.0f}, {0.0f, 1.0f, 0.0f}, 2.0f, 50.0f},
                // Zoom in on red cube
                {{-4.0f, 5.0f, -8.0f}, {-4.0f, 1.0f, -4.0f}, 2.5f, 35.0f},
                // Final pan back out
                {{10.0f, 10.0f, 10.0f}, {0.0f, 0.0f, 0.0f}, 3.0f, 45.0f}
            };

            camCtrl->StartCutscene(cutsceneWaypoints);
            TraceLog(LOG_INFO, "Cutscene started from debug menu");
        } });

    for (const auto &sceneName : availableScenes)
    {
        debugMenu.AddButton("Scene: " + sceneName, [this, sceneName]()
                            {
        if (!SwitchToScene(sceneName))
        {
            TraceLog(LOG_ERROR, "Game: Scene switch failed: %s", sceneName.c_str());
        }
        else
        {
            TraceLog(LOG_INFO, "Game: Switched to scene '%s'", sceneName.c_str());
        } });
    }

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

    if (customModel.getModelCount() == 0)
    {
        customModel.addModel("Rat", "assets/models/rat.obj", "assets/textures/rat.png", {0.04f, 0.04f, 0.04f}, {0.0f, 0.0f, 0.0f});
        customModel.addModel("Miku", "assets/models/miku/scene.gltf", "", {1.8f, 1.8f, 1.8f}, {90.0f, 0.0f, 0.0f});
    }

    customModel.loadPlayerModel(player, currentModelIndex);
}

Model Game::CreateModelFromType(const std::string &modelType)
{
    // Check if this is an external model file (starts with assets/ or ends with .glb)
    if (modelType.find("assets/") == 0 || modelType.find(".glb") != std::string::npos)
    {
        TraceLog(LOG_INFO, "Loading external model: %s", modelType.c_str());
        try
        {
            Model model = LoadModel(modelType.c_str());
            if (model.meshCount > 0)
            {
                TraceLog(LOG_INFO, "Successfully loaded external model with %d meshes", model.meshCount);
                return model;
            }
            else
            {
                TraceLog(LOG_WARNING, "Failed to load external model: %s (no meshes loaded), falling back to cube", modelType.c_str());
                return LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
            }
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, "Exception loading external model '%s': %s, falling back to cube", modelType.c_str(), e.what());
            return LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
        }
        catch (...)
        {
            TraceLog(LOG_ERROR, "Unknown exception loading external model '%s', falling back to cube", modelType.c_str());
            return LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
        }
    }
    // Parse model type string
    else if (modelType.find("plane") == 0)
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

void Game::SetupModels(Scene &scene)
{
    GeometryRenderer *geoRenderer = renderManager.GetGeometryRenderer();
    std::vector<RenderManager::ModelMaterial> modelsToShader;
    std::unordered_set<std::string> shaderConfiguredTypes;

    for (const auto &objData : scene.GetObjects())
    {
        // Create model if not already created
        if (sceneModels.find(objData.modelType) == sceneModels.end())
        {
            sceneModels[objData.modelType] = CreateModelFromType(objData.modelType);
        }

        Model *model = &sceneModels[objData.modelType];

        // Register model type with geometry renderer if not already registered
        if (modelIDs.find(objData.modelType) == modelIDs.end())
        {
            int modelID = geoRenderer->RegisterModel(objData.modelType, model);
            modelIDs[objData.modelType] = modelID;
        }

        // Static objects are batched and culled by GeometryRenderer
        if (objData.mobility == LevelData::ObjectData::Mobility::Static)
        {
            int modelID = modelIDs[objData.modelType];
            geoRenderer->AddInstance(modelID, objData.position, GetUniformScale(objData.scale), objData.rotation);
        }

        // Configure each model type once (applies to static and dynamic objects)
        if (shaderConfiguredTypes.insert(objData.modelType).second)
        {
            // Use -1.0 as sentinel to indicate "use GLB default"
            Vector4 albedoVec = ColorNormalize(objData.albedo);
            float metallic = objData.metallic >= 0.0f ? objData.metallic : -1.0f;
            float roughness = objData.roughness >= 0.0f ? objData.roughness : -1.0f;
            modelsToShader.push_back({model, albedoVec, metallic, roughness});
        }
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

    // Cycle display mode: windowed -> borderless fullscreen -> fullscreen
    if (IsKeyPressed(KEY_F11))
    {
        switch (fullscreenMode)
        {
        case 0: fullscreenMode = 2; break; // Windowed -> Borderless
        case 2: fullscreenMode = 1; break; // Borderless -> Fullscreen
        default: fullscreenMode = 0; break; // Fullscreen/unknown -> Windowed
        }
    }

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

void Game::UpdatePlayerMovement(float deltaTime)
{
    // Only move the player when the camera is in follow mode (gameplay mode).
    // In free-camera mode the player stays put and the camera flies independently.
    CameraController *camCtrl = renderManager.GetCameraController();
    if (!camCtrl || camCtrl->mode != CAMERA_MODE_FOLLOW)
        return;

    // Clamp deltaTime to avoid physics explosions after a hitch
    if (deltaTime > 0.1f)
        deltaTime = 0.1f;

    Vector3 &pos = player.GetTransform().position;

    // -----------------------------------------------------------------------
    // 1. Gather camera-relative movement input (WASD)
    // -----------------------------------------------------------------------
    // Compute a horizontal forward/right basis from the camera's look direction
    Camera3D cam = camCtrl->camera;
    Vector3 camForward = Vector3Subtract(cam.target, cam.position);
    camForward.y = 0.0f; // project onto XZ plane
    float forwardLen = Vector3Length(camForward);
    if (forwardLen > 1e-6f)
        camForward = Vector3Scale(camForward, 1.0f / forwardLen);
    else
        camForward = {0.0f, 0.0f, -1.0f};

    Vector3 camRight = Vector3CrossProduct(camForward, {0.0f, 1.0f, 0.0f});
    camRight = Vector3Normalize(camRight);

    Vector3 moveDir = {0.0f, 0.0f, 0.0f};
    if (IsKeyDown(KEY_W))
        moveDir = Vector3Add(moveDir, camForward);
    if (IsKeyDown(KEY_S))
        moveDir = Vector3Subtract(moveDir, camForward);
    if (IsKeyDown(KEY_D))
        moveDir = Vector3Add(moveDir, camRight);
    if (IsKeyDown(KEY_A))
        moveDir = Vector3Subtract(moveDir, camRight);

    float moveDirLen = Vector3Length(moveDir);
    if (moveDirLen > 1e-6f)
    {
        moveDir = Vector3Scale(moveDir, 1.0f / moveDirLen); // normalize

        float speed = playerMoveSpeed;
        if (IsKeyDown(KEY_LEFT_SHIFT))
            speed *= player.sprintMultiplier;

        pos.x += moveDir.x * speed * deltaTime;
        pos.z += moveDir.z * speed * deltaTime;

        // Rotate player model to face movement direction
        player.playerYaw = atan2f(moveDir.x, moveDir.z) * RAD2DEG;
    }

    // -----------------------------------------------------------------------
    // 2. Apply gravity
    // -----------------------------------------------------------------------
    playerVerticalVelocity -= playerGravity * deltaTime;
    pos.y += playerVerticalVelocity * deltaTime;

    // -----------------------------------------------------------------------
    // 3. Resolve collisions via the SDF
    // -----------------------------------------------------------------------
    if (enableCollision && collisionSystem.IsReady())
    {
        Vector3 resolved = collisionSystem.ResolvePosition(pos, playerCollisionRadius, 4);

        // If the resolved position is higher than where we were heading, we
        // landed on a surface – zero out downward velocity so we don't
        // accumulate gravity while standing on the ground.
        if (resolved.y > pos.y + 1e-4f && playerVerticalVelocity < 0.0f)
            playerVerticalVelocity = 0.0f;

        pos = resolved;
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
    DrawText("WASD: Move | Mouse: Look | E: Talk | TAB: Cursor | ESC: Pause | DELETE: Exit | TAB: Debug Menu",
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

    // SDF collision stats
    if (collisionSystem.IsReady())
    {
        const char *collisionStatus = enableCollision ? "ON" : "OFF";
        Color collisionColor = enableCollision ? LIME : RED;
        DrawText(TextFormat("SDF Collision: %s (res %d)", collisionStatus, collisionSystem.GetResolution()),
                 GetScreenWidth() - 300, UI_MARGIN + 65, UI_SMALL_TEXT_SIZE, collisionColor);

        SDFCollisionResult probe = collisionSystem.QueryCollision(
            player.GetTransform().position, playerCollisionRadius);
        DrawText(TextFormat("Player SDF dist: %.3f %s",
                            probe.penetrationDepth,
                            probe.colliding ? "COLLIDING" : ""),
                 GetScreenWidth() - 300, UI_MARGIN + 85, UI_SMALL_TEXT_SIZE,
                 probe.colliding ? RED : GREEN);
    }

    // Draw debug menus
    debugMenu.Draw();
    postProcessingMenu.Draw();

    if (rmlReady)
    {
        RaylibRmlUi::Draw();
    }
}
