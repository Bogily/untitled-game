/**
 * @file Game.h
 * @brief Main game controller managing game loop and state
 */

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

/**
 * @brief Main game class coordinating all game systems
 *
 * The Game class is the central controller that manages:
 * - Game state transitions (menu, playing, paused, settings)
 * - Core game systems (rendering, world, scenes, UI)
 * - Player and NPC management
 * - Input handling and window management
 */
class Game
{
private:
    static const int SCREEN_WIDTH = 1280; ///< Default screen width
    static const int SCREEN_HEIGHT = 720; ///< Default screen height
    static const int TARGET_FPS = 5000;   ///< Target frame rate limit

    GameState currentState = GameState::MAIN_MENU; ///< Current game state
    MainMenu mainMenu;                             ///< Main menu instance
    PauseMenu pauseMenu;                           ///< Pause menu instance

    Player player;               ///< Player entity
    RenderManager renderManager; ///< Centralized rendering system
    World::WorldManager world;   ///< ECS world manager
    SceneManager sceneManager;   ///< Scene management system
    CustomModel customModel;     ///< Custom model utilities

    SettingsMenu settingsMenu;             ///< Settings menu instance
    DebugMenu debugMenu;                   ///< Debug menu for development
    PostProcessingMenu postProcessingMenu; ///< Post-processing configuration menu

    bool sceneInitialized = false;                      ///< Whether scene has been initialized
    std::unordered_map<std::string, Model> sceneModels; ///< Scene-specific models
    std::unordered_map<std::string, int> modelIDs;      ///< Model ID lookup table

    bool rmlReady = false;                       ///< RmlUI initialization status
    Rml::ElementDocument *rmlMainMenu = nullptr; ///< RmlUI main menu document

    RenderManager::FrameSettings renderFrameSettings; ///< Per-frame rendering configuration

    bool showGrid = true;                 ///< Whether to show debug grid
    bool showGrass = true;                ///< Whether to render grass
    bool showFPS = true;                  ///< Whether to show FPS counter
    float geometryCullMargin = 1.0f;      ///< Geometry frustum culling margin
    float grassCullMargin = 1.70f;        ///< Grass frustum culling margin
    bool enableGrayscale = false;         ///< Grayscale post-processing toggle
    bool enableDepthDebug = false;        ///< Depth buffer visualization toggle
    int msaaLevelIndex = 0;               ///< MSAA level selection index
    int colorGradingPreset = 0;           ///< Color grading preset index
    float colorGradingIntensity = 1.0f;   ///< Color grading intensity
    bool enableContactShadows = false;    ///< Contact shadows toggle
    float contactShadowDistance = 0.1f;   ///< Contact shadow ray march distance
    int contactShadowSteps = 8;           ///< Contact shadow ray march steps
    float contactShadowThickness = 0.01f; ///< Contact shadow thickness
    float contactShadowIntensity = 0.5f;  ///< Contact shadow intensity
    bool enableSSAO = false;              ///< Screen-space ambient occlusion toggle
    int ssaoNumSamples = 8;               ///< SSAO number of samples
    float ssaoRadius = 0.02f;             ///< SSAO sample radius
    float ssaoBias = 0.001f;              ///< SSAO depth bias
    float ssaoIntensity = 0.5f;           ///< SSAO intensity
    float ssaoContrast = 1.0f;            ///< SSAO contrast

    int currentModelIndex = 0;   ///< Currently selected player model
    int previousModelIndex = -1; ///< Previous player model (for change detection)

    int fullscreenMode = 0;          ///< Display mode: 0=Windowed, 1=Fullscreen, 2=Borderless
    int previousFullscreenMode = -1; ///< Previous display mode (for change detection)

    /**
     * @brief Setup game menus with callbacks
     */
    void SetupMenus();

    /**
     * @brief Update game settings from menu changes
     */
    void UpdateSettings();

    /**
     * @brief Handle window resize events
     */
    void HandleWindowResize();

    /**
     * @brief Render settings menu
     */
    void DrawSettings();

    /**
     * @brief Initialize debug menu with controls
     */
    void SetupDebugMenu();

    /**
     * @brief Initialize post-processing menu
     */
    void SetupPostProcessingMenu();

    /**
     * @brief Initialize scene from loaded data
     */
    void InitializeScene();

    /**
     * @brief Cleanup and unload current scene
     */
    void ShutdownScene();

    /**
     * @brief Setup player entity from level data
     * @param level Level configuration data
     */
    void SetupPlayer(const LevelData &level);

    /**
     * @brief Setup scene models from level data
     * @param level Level configuration data
     */
    void SetupModels(const LevelData &level);

    /**
     * @brief Update NPC entities and interactions
     * @param deltaTime Time elapsed since last frame
     */
    void UpdateNPCs(float deltaTime);

    /**
     * @brief Handle player-NPC interaction input
     */
    void HandleNPCInteraction();

    /**
     * @brief Process player input
     * @param deltaTime Time elapsed since last frame
     */
    void HandleInput(float deltaTime);

    /**
     * @brief Render game scene
     */
    void DrawScene();

    /**
     * @brief Render UI elements
     */
    void DrawUI();

    /**
     * @brief Create model from type identifier
     * @param modelType Model type string identifier
     * @return Created model instance
     */
    Model CreateModelFromType(const std::string &modelType);

    /**
     * @brief Update main menu state
     */
    void UpdateMainMenu();

    /**
     * @brief Update gameplay state
     */
    void UpdatePlaying();

    /**
     * @brief Update paused state
     */
    void UpdatePaused();
    /**
     * @brief Render main menu screen
     */
    void DrawMainMenu();

    /**
     * @brief Render playing screen
     */
    void DrawPlaying();

    /**
     * @brief Render paused screen
     */
    void DrawPaused();

    /**
     * @brief Change game state
     * @param newState Target game state
     */
    void ChangeState(GameState newState);

    GameState settingsReturnState = GameState::MAIN_MENU; ///< State to return to from settings

public:
    /**
     * @brief Initialize game systems and load resources
     */
    void Init();

    /**
     * @brief Update game state per frame
     */
    void Update();

    /**
     * @brief Render current game state
     */
    void Draw();

    /**
     * @brief Cleanup and shutdown game systems
     */
    void Shutdown();

    /**
     * @brief Check if game should close
     * @return True if game should exit
     */
    bool ShouldClose();

    /**
     * @brief Apply display mode if it has changed
     */
    void ApplyDisplayModeIfChanged();
};
