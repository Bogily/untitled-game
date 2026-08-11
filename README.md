# 3D Game Engine / Prototype (Raylib + C++)

> **Note**: This repository contains an experimental 3D game project originally developed for a school assignment. The project has since been abandoned as development moved to a different codebase. The code quality, structure, and design patterns here are legacy and do not reflect our current coding standards or style. It is preserved here for historical and reference purposes.

## Overview & Features
- **Language & Build**: C++20, CMake
- **Graphics Framework**: Raylib, GLAD, OpenGL
- **Rendering Pipeline**:
  - Forward+ rendering architecture
  - PBR (Physically-Based Rendering) lighting model
  - Screen-Space Ambient Occlusion (SSAO) & Screen-Space Contact Shadows
  - Post-processing pipeline with color grading and 3D LUT support
  - Compute shader driven wind simulation for grass and volumetric clouds
- **Physics & Collision**: Signed Distance Field (SDF) 3D collision system

## Folder Structure

- **src/**: Source code
  - **core/**: Core engine systems (game loop, window management, input handling)
  - **entities/**: Game entities (Player, NPCs, components)
  - **world/**: Level management, terrain, skybox, SDF collision
  - **graphics/**: Rendering pipeline, model loading, shaders, post-processing
  - **audio/**: Sound and music management
  - **utils/**: Helper utilities, math libraries, logging
- **assets/**: Game assets (models, textures, shaders, sound effects)

## Building

### Requirements
- CMake (3.20 or higher)
- C++20 compatible compiler (MSVC, GCC, or Clang)

### Build Instructions
```bash
# 1. Create build directory
mkdir build
cd build

# 2. Configure with CMake
cmake ..

# 3. Compile the project
cmake --build .
```
