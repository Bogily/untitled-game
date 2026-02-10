/**
 * @file Scene.h
 * @brief Base scene interface for game scenes
 */

#pragma once
#include "raylib.h"
#include "Level.h"
#include "actors/NPC.h"
#include <vector>

/**
 * @brief Base class for all game scenes
 *
 * Scenes provide level data and NPCs without containing logic.
 * The game loop is responsible for all updates and rendering.
 */
class Scene
{
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~Scene() = default;

    /**
     * @brief Load scene resources
     */
    virtual void Load() = 0;

    /**
     * @brief Unload scene resources
     */
    virtual void Unload() = 0;

    /**
     * @brief Get scene level data
     * @return Level data reference
     */
    virtual LevelData &GetLevelData() = 0;

    /**
     * @brief Get scene NPCs
     * @return NPC vector reference
     */
    virtual std::vector<NPC> &GetNPCs() = 0;

    /**
     * @brief Check if scene wants to change
     * @return True if should change scene
     */
    virtual bool ShouldChangeScene() const { return false; }

    /**
     * @brief Get name of next scene
     * @return Scene name or nullptr
     */
    virtual const char *GetNextSceneName() const { return nullptr; }
};
