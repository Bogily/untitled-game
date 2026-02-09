#pragma once
#include "raylib.h"
#include "raymath.h"
#include "../entities/Player.h"
#include "../utils/custommodel.h"
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
    Player player;
    RenderManager renderManager;
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

    // RmlUI state
    bool rmlReady = false;
    Rml::ElementDocument *rmlMainMenu = nullptr;

    // Rendering frame settings
    RenderManager::FrameSettings renderFrameSettings;

    // UI-related state (modified by menus and debug systems)
    bool showGrid = true;
    bool showGrass = true;
    bool showFPS = true;
    float geometryCullMargin = 1.0f;
    float grassCullMargin = 1.70f;
    bool enableGrayscale = false;
    bool enableDepthDebug = false;

    // Player model selection
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
    void SetupPostProcessingMenu();

    // Scene setup
    void InitializeScene();
    void ShutdownScene();
    void SetupPlayer(const LevelData &level);
    void SetupModels(const LevelData &level);

    // Game loop helpers
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
