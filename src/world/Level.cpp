#include "Level.h"

LevelData LevelData::CreateTestLevel()
{
    LevelData level;
    level.name = "Test Scene";

    // Camera settings
    level.camera.startPosition = {0.0f, 10.0f, 10.0f};
    level.camera.startTarget = {0.0f, 0.0f, 0.0f};
    level.camera.startFOV = 45.0f;
    level.camera.followDistance = 10.0f;
    level.camera.followHeight = 6.0f;
    level.camera.smoothness = 0.15f;

    // Player start position
    level.playerStartPosition = {0.0f, 0.0f, 0.0f};

    // Ground plane
    level.objects.push_back({"GroundPlane",
                             {0.0f, 0.0f, 0.0f},
                             {0.0f, 0.0f, 0.0f},
                             {1.0f, 1.0f, 1.0f},
                             {77, 77, 77, 255},
                             0.0f,
                             0.8f,
                             "plane_32x32",
                             {0.0f, 0.0f, 0.0f},
                             0.0f,
                             0.0f,
                             "none"});

    // Test sphere
    level.objects.push_back({"TestSphere",
                             {0.0f, 1.0f, 0.0f},
                             {0.0f, 0.0f, 0.0f},
                             {1.0f, 1.0f, 1.0f},
                             {204, 51, 51, 255},
                             1.0f,
                             0.3f,
                             "sphere_1.0",
                             {0.0f, 0.0f, 0.0f},
                             0.0f,
                             0.0f,
                             "none"});

    // Red cube
    level.objects.push_back({"RedCube",
                             {-4.0f, 1.0f, -4.0f},
                             {0.0f, 0.0f, 0.0f},
                             {1.0f, 1.0f, 1.0f},
                             {204, 26, 26, 255},
                             0.2f,
                             0.4f,
                             "cube_2.0",
                             {2.0f, 2.0f, 2.0f},
                             0.0f,
                             0.0f,
                             "box"});

    // Blue tower
    level.objects.push_back({"BlueTower",
                             {4.0f, 1.0f, 4.0f},
                             {0.0f, 0.0f, 0.0f},
                             {1.0f, 1.0f, 1.0f},
                             {26, 51, 204, 255},
                             0.0f,
                             0.3f,
                             "cube_1x4x1",
                             {1.0f, 4.0f, 1.0f},
                             0.0f,
                             0.0f,
                             "box"});

    // Yellow sphere
    level.objects.push_back({"YellowSphere",
                             {-6.0f, 1.5f, 2.0f},
                             {0.0f, 0.0f, 0.0f},
                             {1.0f, 1.0f, 1.0f},
                             {230, 230, 51, 255},
                             0.0f,
                             0.6f,
                             "sphere_1.5",
                             {0.0f, 0.0f, 0.0f},
                             1.5f,
                             0.0f,
                             "sphere"});

    // Orange sphere
    level.objects.push_back({"OrangeSphere",
                             {6.0f, 1.0f, -2.0f},
                             {0.0f, 0.0f, 0.0f},
                             {1.0f, 1.0f, 1.0f},
                             {230, 128, 26, 255},
                             0.1f,
                             0.3f,
                             "sphere_1.0",
                             {0.0f, 0.0f, 0.0f},
                             1.0f,
                             0.0f,
                             "sphere"});

    // Capsule
    level.objects.push_back({"Capsule",
                             {0.0f, 2.0f, 6.0f},
                             {0.0f, 0.0f, 0.0f},
                             {1.0f, 1.0f, 1.0f},
                             {77, 179, 230, 255},
                             0.0f,
                             0.5f,
                             "cylinder_0.5x3.0",
                             {0.0f, 0.0f, 0.0f},
                             0.5f,
                             3.0f,
                             "capsule"});

    // Cylinder
    level.objects.push_back({"Cylinder",
                             {-2.0f, 2.0f, -6.0f},
                             {0.0f, 0.0f, 0.0f},
                             {1.0f, 1.0f, 1.0f},
                             {128, 51, 204, 255},
                             0.0f,
                             0.4f,
                             "cylinder_0.8x4.0",
                             {0.0f, 0.0f, 0.0f},
                             0.8f,
                             4.0f,
                             "cylinder"});

    // Main ramp
    level.objects.push_back({"MainRamp",
                             {8.0f, 1.5f, -7.0f},
                             {26.565f, 0.0f, 0.0f}, // atan2(3.0, 6.0) * RAD2DEG
                             {1.0f, 1.0f, 1.0f},
                             {102, 64, 38, 255},
                             0.0f,
                             0.7f,
                             "cube_4x0.5x6",
                             {4.0f, 0.5f, 6.0f},
                             0.0f,
                             0.0f,
                             "box"});

    // Steep ramp
    level.objects.push_back({"SteepRamp",
                             {-8.0f, 2.5f, 8.0f},
                             {45.0f, 0.0f, 0.0f}, // atan2(4.0, 4.0) * RAD2DEG
                             {1.0f, 1.0f, 1.0f},
                             {128, 26, 26, 255},
                             0.0f,
                             0.6f,
                             "cube_3x0.5x4",
                             {3.0f, 0.5f, 4.0f},
                             0.0f,
                             0.0f,
                             "box"});

    // Ramp walls
    level.objects.push_back({"RampWallLeft",
                             {5.85f, 1.5f, -7.0f},
                             {26.565f, 0.0f, 0.0f},
                             {1.0f, 1.0f, 1.0f},
                             {77, 51, 26, 255},
                             0.0f,
                             0.8f,
                             "cube_0.3x3x6",
                             {0.3f, 3.0f, 6.0f},
                             0.0f,
                             0.0f,
                             "box"});

    level.objects.push_back({"RampWallRight",
                             {10.15f, 1.5f, -7.0f},
                             {26.565f, 0.0f, 0.0f},
                             {1.0f, 1.0f, 1.0f},
                             {77, 51, 26, 255},
                             0.0f,
                             0.8f,
                             "cube_0.3x3x6",
                             {0.3f, 3.0f, 6.0f},
                             0.0f,
                             0.0f,
                             "box"});

    level.objects.push_back({"SteepRampWallLeft",
                             {-9.65f, 2.5f, 8.0f},
                             {45.0f, 0.0f, 0.0f},
                             {1.0f, 1.0f, 1.0f},
                             {89, 38, 26, 255},
                             0.0f,
                             0.75f,
                             "cube_0.3x5x4",
                             {0.3f, 5.0f, 4.0f},
                             0.0f,
                             0.0f,
                             "box"});

    level.objects.push_back({"SteepRampWallRight",
                             {-6.35f, 2.5f, 8.0f},
                             {45.0f, 0.0f, 0.0f},
                             {1.0f, 1.0f, 1.0f},
                             {89, 38, 26, 255},
                             0.0f,
                             0.75f,
                             "cube_0.3x5x4",
                             {0.3f, 5.0f, 4.0f},
                             0.0f,
                             0.0f,
                             "box"});

    // NPCs
    level.npcs.push_back({"Rat Merchant",
                          {8.0f, 0.0f, 2.0f},
                          {"Hello traveler!", "I sell the finest wares in all the land!", "Care to make a purchase?"},
                          3.0f});

    level.npcs.push_back({"Miku the Wise",
                          {-8.0f, 0.0f, -2.0f},
                          {"Greetings, young one.", "The path ahead is perilous.", "Beware the darkness that lurks beyond."},
                          3.0f});

    // Lights
    level.lights.push_back({"Sun",
                            0, // directional
                            {0.0f, 0.0f, 0.0f},
                            {-0.3f, -1.0f, -0.4f},
                            {255, 244, 229, 255},
                            1.0f,
                            0.0f});

    level.lights.push_back({"RedLight",
                            1, // point
                            {-4.0f, 3.0f, -4.0f},
                            {0.0f, 0.0f, 0.0f},
                            {255, 100, 100, 255},
                            2.5f,
                            15.0f});

    level.lights.push_back({"BlueLight",
                            1, // point
                            {4.0f, 5.0f, 4.0f},
                            {0.0f, 0.0f, 0.0f},
                            {100, 150, 255, 255},
                            2.8f,
                            18.0f});

    level.lights.push_back({"YellowLight",
                            1, // point
                            {-6.0f, 3.0f, 2.0f},
                            {0.0f, 0.0f, 0.0f},
                            {255, 255, 150, 255},
                            2.6f,
                            16.0f});

    level.lights.push_back({"WhiteLight",
                            1, // point
                            {0.0f, 8.0f, 0.0f},
                            {0.0f, 0.0f, 0.0f},
                            {255, 255, 255, 255},
                            1.5f,
                            25.0f});

    // Grass settings
    level.grassPosition = {0.0f, 0.0f, 0.0f};
    level.grassWidth = 30.0f;
    level.grassLength = 30.0f;
    level.grassBladeCount = 10000;

    // Water settings
    level.waterPosition = {0.0f, 0.5f, 0.0f};
    level.waterWidth = 20.0f;
    level.waterLength = 20.0f;

    // Skybox
    level.skyboxTexture = "assets/textures/skybox.png";

    return level;
}
