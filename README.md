# Zelda-like 3D Game Structure (Raylib + C++)

This project provides a foundational folder structure for a 3D third-person adventure game using Raylib and C++.

## Folder Structure

- **src/**: Contains all source code.
  - **core/**: Core engine systems (Game loop, Window management, Input handling).
  - **entities/**: Game entities (Player, Enemies, NPCs).
  - **world/**: Level management, Terrain, Skybox.
  - **graphics/**: Rendering systems, Model loading, Shaders.
  - **audio/**: Sound and Music management.
  - **utils/**: Helper functions, Math libraries, Logging.
- **assets/**: Game assets.
  - **models/**: 3D models (.obj, .gltf, etc.).
  - **textures/**: Image textures (.png, .jpg).
  - **sounds/**: Audio files (.wav, .ogg).

## Building

This project uses CMake.

1. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```
2. Configure the project:
   ```bash
   cmake ..
   ```
3. Build:
   ```bash
   make
   ```
4. Run:
   ```bash
   ./ZeldaLikeGame
   ```

## Dependencies

- **Raylib**: Automatically fetched via CMake.
