/**
 * @file ActorRenderer.h
 * @brief Renderer for actor entities (player and NPCs)
 */

#pragma once

#include "raylib.h"

class Player;
class Scene;

/**
 * @brief Handles rendering of all actor entities
 */
class ActorRenderer
{
public:
    /**
     * @brief Draw player and scene NPCs
     * @param player Player instance
     * @param scene Current scene (may be nullptr)
     * @param renderCamera Active render camera
     */
    void Draw(Player &player, Scene *scene, const Camera3D &renderCamera);
};
