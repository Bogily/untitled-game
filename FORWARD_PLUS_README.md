# Forward+ Rendering System Implementation

## Overview

This project now includes a **Forward+ (Tiled Forward Rendering)** system as an alternative to the standard Forward PBR rendering pipeline. Forward+ is an advanced rendering technique that efficiently handles many lights by using compute shaders to cull lights per screen-space tile.

## Architecture

### Key Components

1. **ForwardPlusSystem** (`src/graphics/ForwardPlusSystem.h/cpp`)

   - Manages the Forward+ rendering pipeline
   - Handles up to 128 lights (vs. 4 in standard PBR)
   - Performs GPU-accelerated light culling using compute shaders
   - Supports both point lights and directional lights

2. **Light Culling Compute Shader** (`assets/shader/light_culling.comp`)

   - Divides screen into 16x16 pixel tiles
   - Culls lights per tile using frustum intersection tests
   - Creates light lists for efficient fragment shader access
   - Uses Shader Storage Buffer Objects (SSBOs) for data exchange

3. **Forward+ PBR Shaders** (`assets/shader/pbr_forward_plus.vs/fs`)

   - Vertex shader: Standard vertex transformation
   - Fragment shader:
     - Looks up which lights affect current tile
     - Processes only visible lights
     - Uses Cook-Torrance BRDF for physically-based shading

4. **RenderPipeline Integration** (`src/graphics/RenderPipeline.h/cpp`)
   - Toggle between standard PBR and Forward+ at runtime
   - Preserves existing functionality
   - No breaking changes to existing code

## Usage

### Toggle Forward+ Rendering

1. **Runtime Toggle**: Press `F1` to open Debug Menu
2. Enable **"Enable Forward+ Rendering"** checkbox
3. The system will automatically switch shaders on all PBR models

### Programmatic Control

```cpp
// Enable Forward+ rendering
renderPipeline.EnableForwardPlus(true);

// Check if enabled
if (renderPipeline.IsForwardPlusEnabled()) {
    // Forward+ is active
}

// Get Forward+ system for configuration
ForwardPlusSystem* fp = renderPipeline.GetForwardPlus();
```

### Adding Lights

#### Point Light

```cpp
gForwardPlus.CreatePointLight(
    Vector3{x, y, z},        // Position
    Vector4{r, g, b, a},     // Color
    intensity,               // Intensity
    radius                   // Light radius (for culling)
);
```

#### Directional Light

```cpp
gForwardPlus.CreateDirectionalLight(
    Vector3{x, y, z},        // Direction
    Vector4{r, g, b, a},     // Color
    intensity                // Intensity
);
```

## Performance Characteristics

### Advantages

- **Many Lights**: Supports up to 128 lights (vs. 4 in standard forward)
- **Efficient Culling**: Only processes lights that affect each screen tile
- **Scalable**: Performance scales better with light count
- **Quality**: Same PBR quality as standard forward rendering

### Technical Details

- **Tile Size**: 16x16 pixels
- **Max Lights**: 128 total lights
- **Max Lights Per Tile**: 256 lights
- **SSBO Bindings**:
  - Binding 0: Light data buffer
  - Binding 1: Visible light indices buffer
  - Binding 2: Light grid buffer (offset + count per tile)

## Compatibility

### Requirements

- OpenGL 4.3+ (for compute shaders and SSBOs)
- GLAD for OpenGL function loading
- Raylib for rendering framework

### Fallback

If Forward+ is disabled or fails to initialize, the system automatically falls back to standard PBR rendering with no loss of functionality.

## Implementation Details

### Light Culling Pipeline

1. **Compute Dispatch**: One workgroup per tile (16x16 threads)
2. **Frustum Construction**: Build tile frustum in view space
3. **Light Testing**: Test each light against tile frustum
4. **Index Generation**: Build per-tile light index lists
5. **Fragment Access**: Fragment shader reads tile's light list

### Shader Data Flow

```
CPU (Game.cpp)
  ↓
ForwardPlusSystem::Update()
  ↓
Light Culling Compute Shader (light_culling.comp)
  ↓ (writes to SSBOs)
Forward+ Fragment Shader (pbr_forward_plus.fs)
  ↓
Final Rendered Image
```

## Files Modified/Created

### New Files

- `src/graphics/ForwardPlusSystem.h`
- `src/graphics/ForwardPlusSystem.cpp`
- `assets/shader/light_culling.comp`
- `assets/shader/pbr_forward_plus.vs`
- `assets/shader/pbr_forward_plus.fs`

### Modified Files

- `src/graphics/RenderPipeline.h` - Added Forward+ support
- `src/graphics/RenderPipeline.cpp` - Integration logic
- `src/core/Game.h` - Added Forward+ toggle and helper methods
- `src/core/Game.cpp` - Initialization, update, and mode switching
- `src/utils/DebugMenu.*` - Added Forward+ toggle option

### No Changes Required

- `CMakeLists.txt` - Already uses GLOB_RECURSE for automatic file discovery
- Existing PBR shaders - Preserved for backward compatibility
- Other rendering systems - Water, grass, skybox, etc. unaffected

## Testing

### Verification Steps

1. Build the project: `cd build && mingw32-make`
2. Run the game: `.\build\ZeldaLikeGame.exe`
3. Press `F1` to open Debug Menu
4. Toggle "Enable Forward+ Rendering"
5. Verify smooth transition between rendering modes
6. Observe identical visual quality
7. Check console for initialization messages

### Expected Console Output

```
[INFO] ForwardPlusSystem: Screen 1280x720, Tiles 80x45
[INFO] ForwardPlusSystem: Shaders loaded (PBR: X, Culling: Y)
[INFO] ForwardPlusSystem: Created compute buffers (Tiles: 3600)
[INFO] ForwardPlusSystem: Initialized successfully
[INFO] Game: Forward+ lights configured
[INFO] Game: Applied Forward+ shaders to all models
```

## Future Enhancements

Potential improvements:

- Debug visualization of light tiles
- Dynamic light radius adjustment
- Shadow mapping integration
- Clustered shading (3D tiles instead of 2D)
- Light culling optimization (min/max depth)
- Per-light type optimization

## Troubleshooting

### Forward+ Not Working

- Check OpenGL version: Must be 4.3+
- Verify compute shader compilation
- Check SSBO bindings in shaders
- Review console for error messages

### Visual Artifacts

- Verify tile size matches between compute and fragment shaders
- Check light data structure alignment (std430 layout)
- Ensure SSBOs are bound correctly

### Performance Issues

- Reduce max lights if needed
- Increase tile size (trade accuracy for performance)
- Profile compute shader dispatch time
- Check for unnecessary buffer uploads

## Credits

Implemented as a non-breaking addition to the existing PBR rendering system, preserving all original functionality while adding advanced lighting capabilities.
