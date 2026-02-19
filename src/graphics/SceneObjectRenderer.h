/**
 * @file SceneObjectRenderer.h
 * @brief Renderer for scene-level dynamic objects
 */

#pragma once

#include "raylib.h"
#include "rlgl.h"
#include "../world/Scene.h"
#include <string>
#include <unordered_map>

/**
 * @brief Handles rendering of dynamic scene objects
 */
class SceneObjectRenderer
{
public:
    /**
     * @brief Draw all dynamic objects from the active scene
     * @param scene Current scene (may be nullptr)
     * @param sceneModels Model lookup table keyed by object model type
     */
    void DrawDynamicObjects(Scene *scene, const std::unordered_map<std::string, Model> &sceneModels) const
    {
        if (!scene)
            return;

        for (const auto &objData : scene->GetObjects())
        {
            if (objData.mobility != LevelData::ObjectData::Mobility::Dynamic)
                continue;

            auto modelIt = sceneModels.find(objData.modelType);
            if (modelIt == sceneModels.end())
                continue;

            rlPushMatrix();
            rlTranslatef(objData.position.x, objData.position.y, objData.position.z);
            rlRotatef(objData.rotation.x, 1.0f, 0.0f, 0.0f);
            rlRotatef(objData.rotation.y, 0.0f, 1.0f, 0.0f);
            rlRotatef(objData.rotation.z, 0.0f, 0.0f, 1.0f);
            rlScalef(objData.scale.x, objData.scale.y, objData.scale.z);
            DrawModel(modelIt->second, {0, 0, 0}, 1.0f, WHITE);
            rlPopMatrix();
        }
    }
};
