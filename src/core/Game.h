#pragma once
#include "raylib.h"
#include "raymath.h"
#include "../player/Player.h"
#include "../physics/CollisionSystem.h"
#include "../utils/custommodel.h"
#include "../utils/DebugMenu.h"
#include "../graphics/Skybox.h"
#include "../graphics/BillboardText.h"
#include "../graphics/CameraController.h"
#include "../graphics/Renderer.h"
#include "../graphics/GrassRenderer.h"
#include "GameState.h"
#include <vector>

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

    // Game constants
    static constexpr float FIXED_CAMERA_MOVE_SPEED = 5.0f;
    static constexpr float MIN_PLAYER_HEIGHT = 1.0f;
    static constexpr float MAX_VERTICAL_MOVE_HEIGHT = 5.0f;
    static constexpr int UI_TEXT_SIZE = 20;
    static constexpr int UI_SMALL_TEXT_SIZE = 16;
    static constexpr int UI_MARGIN = 10;
    static constexpr int UI_LINE_SPACING = 30;

    // Game objects
    CameraController cameraController;
    Player player;
    CustomModel customModel;
    Skybox skybox;
    DebugMenu debugMenu;
    SettingsMenu settingsMenu;
    Renderer renderer;
    CollisionSystem collisionSystem;
    GrassRenderer grassRenderer;
    Mesh slopeMesh;
    Model slopeModel;
    Model pbrTestSphere;

    // PBR world objects
    Model pbrRedCube;
    Model pbrBlueTower;
    Model pbrYellowSphere;
    Model pbrOrangeSphere;
    Model pbrCapsule;
    Model pbrCylinder;
    Model pbrGroundPlane;
    Model pbrRamp;
    Model pbrSteepRamp;

    // Debug flags
    bool showGrid = true;
    bool showRaycast = true;
    bool showPlayerPos = false;
    bool showFPS = true;
    bool showCollisionBoxes = true;
    bool showPlayerHitbox = true;
    bool showGrass = true;

    // Model selection
    int currentModelIndex = 0;
    int previousModelIndex = 0;
    // Fullscreen/window mode: 0=Windowed,1=Fullscreen,2=Borderless (windowed fullscreen)
    int fullscreenMode = 0;
    int previousFullscreenMode = -1;

    // Helper methods
    void SetupCamera();
    void SetupPlayer();
    void SetupModels();
    void SetupSkybox();
    void SetupRenderer();
    void SetupGrass();
    void SetupCollisions();

    void SetupDebugMenu();
    void UpdateSettings();
    void DrawSettings();
    void HandleInput(float deltaTime);
    void HandleCameraControls();
    void UpdatePlayer(float deltaTime);
    void UpdatePlayerInFixedCamera(float deltaTime);
    void DrawScene();
    void DrawUI();
    void Draw3DBillboards();
    void Draw2DUI();

    // Camera cutscene helpers
    void StartOverviewCutscene();
    void StartZoomCutscene();
    void StartOrbitCutscene(const Vector3 &target, float radius, int segments);
    std::vector<CameraWaypoint> CreateCircularOrbit(const Vector3 &center, float radius, float height, int segments, float duration, float fov);

    // State management
    void SetupMenus();
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
