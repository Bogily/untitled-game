# Entity Component System

## What is this?

Data-oriented ECS for better performance and cleaner code. Instead of scattering object data everywhere, components are grouped by type in contiguous arrays.

## Why?

**Better cache usage**  
CPU can prefetch what it needs instead of loading entire objects.

**Easier to maintain**  
Add an entity once. Systems automatically pick it up.

**SIMD-ready**  
Process multiple entities at once with vector instructions.

## How it works

```
Entity (just an ID)
    ↓
Components (arrays of data)
    - positions[], rotations[], scales[]
    - models[], colors[], materials[]
    - shapes[], sizes[], radii[]
    ↓
Systems (process components)
    - Iterate arrays directly
    - No virtual calls
    - Cache-friendly
```

## Usage

```cpp
// Create an entity
Entity e = world.CreateEntity();

// Add components
world.AddTransform(e, {0, 1, 0});
world.AddRender(e, &model, modelID, color, 0.5f, 0.3f);
world.AddCollision(e, COLLISION_BOX, {2,2,2}, 0,0, {0,0,0}, RED);
```

Done. Rendering and collision systems will find it automatically.

## Memory layout

**Old way (bad for cache):**

```
[Object0: pos,rot,model,color] [Object1: pos,rot,model,color] ...
```

**New way (cache-friendly):**

```
positions: [pos0][pos1][pos2][pos3]...
rotations: [rot0][rot1][rot2][rot3]...
models:    [mdl0][mdl1][mdl2][mdl3]...
```

When you need positions, CPU loads just positions. No wasted cache space.

## Example

```cpp
// Get all entities that can be rendered
auto entities = world.GetEntitiesWithComponents(
    COMPONENT_TRANSFORM | COMPONENT_RENDER
);

// Process them
const auto& transforms = world.GetTransforms();
const auto& renders = world.GetRenders();

for (Entity e : entities) {
    int tIdx = world.GetTransformIndex(e);
    int rIdx = world.GetRenderIndex(e);

    Vector3 pos = transforms.positions[tIdx];
    Model* model = renders.models[rIdx];

    DrawModel(*model, pos, 1.0f, WHITE);
}
```

Sequential memory access = fast.

## Files

- `Entity.h` - Entity IDs and component flags
- `Components.h` - Component data (Structure of Arrays)
- `World.h` - Entity/component manager
- `Game.cpp` - See SetupModels() for examples

## What's next

- Multi-threading: Split arrays across threads
- SIMD: Process 4-8 entities per iteration
- Archetypes: Group entities by component combo for even better locality
