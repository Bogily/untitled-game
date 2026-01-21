#pragma once
#include "raylib.h"
#include "Level.h"
#include "entities/NPC.h"
#include <vector>

// Base class for all game scenes - provides ONLY data, no logic
class Scene
{
public:
    virtual ~Scene() = default;

    // Scene lifecycle (for loading/unloading resources)
    virtual void Load() = 0;
    virtual void Unload() = 0;

    // Data access - scene provides data, game loop uses it
    virtual LevelData &GetLevelData() = 0;
    virtual std::vector<NPC> &GetNPCs() = 0;

    // Scene state queries
    virtual bool ShouldChangeScene() const { return false; }
    virtual const char *GetNextSceneName() const { return nullptr; }
};
