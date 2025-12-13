# Copilot Instructions for Zelda-like 3D Game

## Architecture Overview

This is a 3D third-person adventure game built with **Raylib** (C++) using a component-based architecture with clear separation of concerns:

- **Core Game Loop**: [src/core/Game.cpp](src/core/Game.cpp) owns all major systems and orchestrates Init → Update → Draw → Shutdown lifecycle
- **Player System**: Player state lives in [src/player/Player.h](src/player/Player.h), movement logic in [src/player/movement.cpp](src/player/movement.cpp)
- **Camera System**: [src/graphics/CameraController.h](src/graphics/CameraController.h) supports 4 modes (Free/Follow/Cutscene/Fixed) with smooth transitions and waypoint-based cinematics
- **Model System**: [src/utils/custommodel.h](src/utils/custommodel.h) manages multiple player models with per-model scale/rotation offsets
- **Skybox**: Procedural shader-based skybox in [src/graphics/Skybox.h](src/graphics/Skybox.h)
- **Debug Menu**: Runtime-editable settings via [src/utils/DebugMenu.h](src/utils/DebugMenu.h) (toggle with TAB)

## Key Patterns

### Constants Over Magic Numbers

Define game constants in namespaces (see `GameConstants` in [Game.cpp](src/core/Game.cpp#L5-L17)):

```cpp
namespace GameConstants {
    constexpr Vector3 PLAYER_START_POS = {0.0f, 1.0f, 0.0f};
    constexpr float CAMERA_FOLLOW_DISTANCE = 10.0f;
}
```

### Frame-Rate Independent Movement

**Always** multiply speeds by `GetFrameTime()`:

```cpp
float speed = 5.0f; // units per second
float moveAmount = speed * GetFrameTime();
```

### Camera Mode Switching

Key bindings 1-6 switch camera modes (see [Game.cpp](src/core/Game.cpp#L125-L163)). When adding camera features, use `CameraController::TransitionTo()` for smooth interpolation.

### Player Model Management

Models are registered via `CustomModel::addModel()` with name, path, scale, and rotation offsets. Switch models by index without manual cleanup—the system handles model unloading.

### Debug Menu Integration

Add any runtime-tweakable value to the debug menu in `SetupDebugMenu()`:

```cpp
debugMenu.AddFloat("Jump Strength", &player.jumpStrength, 1.0f, 20.0f, 0.1f);
debugMenu.AddBool("Show Grid", &showGrid);
```

## Build & Run

```bash
# From project root
mkdir -p build && cd build
cmake ..
make
./ZeldaLikeGame
```

**Raylib is fetched automatically** via CMake FetchContent—no manual installation needed.

## File Organization

- **Header-only for simple classes** (e.g., [Player.h](src/player/Player.h) with separate [movement.cpp](src/player/movement.cpp) for implementation)
- **Assets copied to build/** automatically via `CMakeLists.txt` line 28—reference as `assets/models/...` in code
- **Shaders** live in `assets/shader/` (e.g., `skybox.vs`, `skybox.fs`)

## Commit Convention

Use conventional commits format: `type(scope): description`

- Types: `feat`, `fix`, `refactor`, `docs`, `style`, `test`, `chore`
- Example: `feat(Audio): fixed audio playing in reverse`

See [commit cheat sheet](https://gist.github.com/joshbuchea/6f47e86d2510bce28f8e7f42ae84c716)

## Common Tasks

### Adding a New Game System

1. Create header in appropriate `src/` subfolder (e.g., `src/audio/AudioManager.h`)
2. Include in [Game.h](src/core/Game.h) and add as member variable
3. Initialize in `Game::Init()`, update in `Game::Update()`, cleanup in `Game::Shutdown()`

### Adding Camera Cutscenes

Create waypoints in `Game.cpp` (see `StartOrbitCutscene()` for circular orbit example). Use `CreateCircularOrbit()` helper for camera orbits around objects.

### Asset Loading

Models: `.obj`, `.gltf` (GLTF models may need 90° X-axis rotation offset)
Textures: `.png`, `.jpg` in `assets/textures/`
Reference paths relative to build directory: `"assets/models/rat.obj"`
