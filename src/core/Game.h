#pragma once
#include "raylib.h"
#include "raymath.h"
#include "../player/Player.h"
#include "../physics/CollisionSystem.h"
#include "../utils/custommodel.h"
#include "../utils/DebugMenu.h"
#include "../utils/PostProcessingMenu.h"
#include "../graphics/BillboardText.h"
#include "../graphics/SpeechBubble.h"
#include "../graphics/CameraController.h"
#include "../graphics/RenderManager.h"
#include "../world/NPC.h"
#include "../world/World.h"
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
    DebugMenu debugMenu;
    PostProcessingMenu postProcessingMenu;
    SettingsMenu settingsMenu;
    RenderManager renderManager;
    CollisionSystem collisionSystem;
    World::WorldManager world; // ECS world manager
    Mesh slopeMesh;
    Model slopeModel;
    Model pbrTestSphere;

    // NPCs and speech bubbles
    std::vector<NPC> npcs;
    Graphics::SpeechBubbleManager speechBubbleManager;

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
    Model pbrRampWallLeft;
    Model pbrRampWallRight;
    Model pbrSteepRampWallLeft;
    Model pbrSteepRampWallRight;

    // Geometry renderer model IDs
    int modelID_RedCube;
    int modelID_BlueTower;
    int modelID_YellowSphere;
    int modelID_OrangeSphere;
    int modelID_Capsule;
    int modelID_Cylinder;
    int modelID_GroundPlane;
    int modelID_Ramp;
    int modelID_SteepRamp;
    int modelID_TestSphere;
    int modelID_RampWallLeft;
    int modelID_RampWallRight;
    int modelID_SteepRampWallLeft;
    int modelID_SteepRampWallRight;

    // Debug flags
    bool showGrid = true;
    bool showRaycast = true;
    bool showPlayerPos = false;
    bool showFPS = true;
    bool showCollisionBoxes = true;
    bool showPlayerHitbox = true;
    bool showGrass = true;

    // Gameplay tweakables
    float npcInteractionRange = 3.0f;

    // Post-processing settings (controlled by PostProcessingMenu)
    bool enablePostProcessing = true;
    bool enableGrayscale = false;

    // Culling tuning
    float geometryCullMargin = 1.25f; // radius multiplier (>1.0 less aggressive)
    float grassCullMargin = 1.15f;    // grass radius multiplier

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
    void SetupWater();
    void SetupCollisions();
    void SetupNPCs();

    void SetupDebugMenu();
    void SetupPostProcessingMenu();
    void UpdateSettings();
    void HandleWindowResize();
    void DrawSettings();
    void ApplyRenderingMode(); // Apply PBR shaders to models
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
