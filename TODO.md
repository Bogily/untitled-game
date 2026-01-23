# Important

- Make the Entities Object oriented, base entity class and just add components to each entity (camera component etc)
- Refactor the world folder entities and normal entities folder
- Write the blender Plugins (scene system and collisions export)
  ## Auto-generated colliders
  - Pre-generate colliders in Blender on export for shipped assets.
  - Use primitive fitting (box / sphere / capsule) as a first-pass cheap collider.
  - If primitives are insufficient, compute convex hulls; for complex meshes run V-HACD.
  - Serialize a per-model cache file (e.g. `modelname.colcache`) with model-hash and settings.
  - Engine: load `*.colcache` when present; otherwise use a cheap fallback and queue background generation.
  - Provide quality presets: Fast / Balanced / Accurate (tunable voxel / part limits).
  - Optionally add a headless Blender/CI export step to precompute caches for builds.

---

# Semi Important

---

# Not Important
