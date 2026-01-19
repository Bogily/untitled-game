#pragma once
#include "raylib.h"
#include "raymath.h"
#include "../player/Player.h"
#include "../physics/CollisionSystem.h"
#include "../utils/custommodel.h"
#include "../graphics/CameraController.h"
#include "../graphics/RenderManager.h"
#include "../world/World.h"
#include "../world/SceneManager.h"
#include "../world/Level.h"
#include "../ui/DebugMenu.h"
#include "../ui/PostProcessingMenu.h"
#include "GameState.h"
#include "ui/rmlui/raylibRmlUi.h"
#include <vector>
#include <unordered_map>

class Game
{
private:
    // Screen settings
    static const int SCREEN_WIDTH = 1280;
    static const int SCREEN_HEIGHT = 720;
    static const int TARGET_FPS = 5000;

    // Game state
    GameState currentState = GameState::MAIN_MENU;
    MainMenu mainMenu;
    PauseMenu pauseMenu;

    // Game systems
    CameraController cameraController;
    Player player;
    RenderManager renderManager;
    CollisionSystem collisionSystem;
    World::WorldManager world; // ECS world manager
    SceneManager sceneManager; // Scene management system
    CustomModel customModel;

    // Settings menu
    SettingsMenu settingsMenu;

    // Debug menu
    DebugMenu debugMenu;
    PostProcessingMenu postProcessingMenu;

    // Scene-related state
    bool sceneInitialized = false;
    std::unordered_map<std::string, Model> sceneModels;
    std::unordered_map<std::string, int> modelIDs;

    // Debug/UI state
    bool showGrid = true;
    bool showCollisionBoxes = true;
    bool showPlayerHitbox = true;
    bool showGrass = true;
    bool showFPS = true;
    bool enablePostProcessing = false;
    bool enableGrayscale = false;
    bool rmlReady = false;
    Rml::ElementDocument *rmlMainMenu = nullptr;
    // Culling margins
    float geometryCullMargin = 1.0f;
    float grassCullMargin = 1.70f; // default per request
    int currentModelIndex = 0;
    int previousModelIndex = -1;

    // Fullscreen/window mode: 0=Windowed,1=Fullscreen,2=Borderless (windowed fullscreen)
    int fullscreenMode = 0;
    int previousFullscreenMode = -1;

    // Helper methods
    void SetupMenus();
    void UpdateSettings();
    void HandleWindowResize();
    void DrawSettings();
    void SetupDebugMenu();

    // Scene setup
    void InitializeScene();
    void ShutdownScene();
    void SetupRenderer();
    void SetupCamera(const LevelData &level);
    void SetupPlayer(const LevelData &level);
    void SetupModels(const LevelData &level);
    void SetupLights(const LevelData &level);
    void SetupSkybox(const LevelData &level);
    void SetupGrass(const LevelData &level);
    void SetupWater(const LevelData &level);
    void SetupParticles(const LevelData &level);
    void SetupCollisions(const LevelData &level);

    // Game loop helpers
    void UpdatePlayer(float deltaTime);
    void UpdateNPCs(float deltaTime);
    void HandleNPCInteraction();
    void HandleInput(float deltaTime);
    void DrawScene();
    void DrawUI();

    // Model creation helpers
    Model CreateModelFromType(const std::string &modelType);

    // State management
    void UpdateMainMenu();
    void UpdatePlaying();
    void UpdatePaused();
    void DrawMainMenu();
    void DrawPlaying();
    void DrawPaused();
    void ChangeState(GameState newState);
    // Remember where Settings was opened from so Back returns appropriately
    GameState settingsReturnState = GameState::MAIN_MENU;

public:
    void Init();
    void Update();
    void Draw();
    void Shutdown();
    bool ShouldClose();
    void ApplyDisplayModeIfChanged();
};
