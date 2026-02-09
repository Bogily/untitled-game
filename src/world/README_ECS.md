# Entity Component System Refactored

## Overview

Clean, cache-efficient ECS using Structure of Arrays with O(1) component removal.

## Design Principles

**1. Data-Oriented Layout**
Component data is stored in separate contiguous arrays, bringing better CPU cache behavior and SIMD opportunities.

**2. Efficient Removal**
Uses swap-and-pop pattern (O(1)) instead of erase (O(n)). When removing a component at index i, we swap the last element into position i and pop the end.

**3. Sparse Entity Indexing**
Entities map to component indices via lookup arrays. An entity can have any combination of components without wasting memory.

**4. Simple Component Queries**
Bitmask-based queries are fast and flexible, supporting queries like "all entities with Transform AND Render".

## Memory Layout

```
Entities:           [Info0][Info1][Info2][Info3]...
                    [mask, active][mask, active]...

Transforms:
  positions:        [pos0][pos1][pos2][pos3]...
  rotations:        [rot0][rot1][rot2][rot3]...
  scales:           [scale0][scale1][scale2][scale3]...

Renders:
  models:           [model0][model1][model2]...
  colors:           [color0][color1][color2]...
  materials:        [mat0][mat1][mat2]...

Entity->Component Mapping:
  transformIdx:     [t0][t1][-1][t2]...  (-1 = no component)
  renderIdx:        [-1][r0][r1][-1]...
  metadataIdx:      [m0][-1][m2][m3]...
```

When iterating renders, all model pointers are sequential in memory ✓ Cache friendly!

## Usage Example

```cpp
// Create and setup entity
Entity player = world.CreateEntity();
world.AddMetadata(player, "Player", false);
world.AddTransform(player, {0, 1, 0}, {0, 0, 0}, {1, 1, 1});
world.AddRender(player, &playerModel, modelID, WHITE, 0.5f, 0.3f);

// Get entities for rendering
auto toRender = world.GetEntitiesWithComponents(
    COMPONENT_TRANSFORM | COMPONENT_RENDER
);

// Fast iteration over component data
const auto& transforms = world.GetTransforms();
const auto& renders = world.GetRenders();

for (Entity e : toRender) {
    int tIdx = world.GetTransformIndex(e);
    int rIdx = world.GetRenderIndex(e);

    Vector3 pos = transforms.positions[tIdx];
    Model* mdl = renders.models[rIdx];

    DrawModel(*mdl, pos, 1.0f, WHITE);
}

// Removing is also fast (O(1))
world.RemoveRender(player);  // Swap-and-pop, no array scanning
```

## Performance Notes

| Operation                 | Complexity | Notes                             |
| ------------------------- | ---------- | --------------------------------- |
| CreateEntity              | O(1)       | Reuses from freelist if available |
| DestroyEntity             | O(1)       | Mark inactive, add to freelist    |
| AddComponent              | O(1)       | Just append to arrays             |
| RemoveComponent           | O(1)       | Swap-and-pop, no reindexing       |
| HasComponent              | O(1)       | Bitmap check                      |
| GetEntitiesWithComponents | O(N)       | Linear scan by entity count       |

## Architecture Files

- **Entity.h** - Entity IDs and component type flags
- **Components.h** - Component structs (SoA layout, swap-and-pop removal)
- **World.h** - WorldManager (entity/component manager)
- **Scene.h** - Base scene interface
- **LuaScene.cpp** - Lua-scripted scene loading

## Future Improvements

- Multi-threaded component processing
- Per-component removal callbacks
- Entity serialization
- Archetype-based queries for SIMD operations
