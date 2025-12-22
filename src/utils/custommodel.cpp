#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "../player/Player.h"
#include "custommodel.h"

void CustomModel::addModel(const std::string &name, const std::string &modelPath, const std::string &texturePath, Vector3 scale, Vector3 rotationOffset)
{
    availableModels.push_back({modelPath, texturePath, name, scale, rotationOffset});
}

void CustomModel::loadPlayerModel(Player &player, int modelIndex)
{
    if (modelIndex < 0 || modelIndex >= (int)availableModels.size())
    {
        TraceLog(LOG_WARNING, "Invalid model index: %d", modelIndex);
        return;
    }

    const ModelData &modelData = availableModels[modelIndex];

    // Unload previous model if one exists
    if (player.modelLoaded)
    {
        UnloadModel(player.model);
        player.modelLoaded = false;
    }

    // Load new model
    const char *texPath = modelData.texturePath.empty() ? nullptr : modelData.texturePath.c_str();
    loadPlayerModel(player, modelData.modelPath.c_str(), texPath);

    // Set model scale and rotation
    player.modelScale = modelData.scale;
    player.modelRotationOffset = modelData.rotationOffset;

    TraceLog(LOG_INFO, "Switched to model: %s", modelData.name.c_str());
}

std::string CustomModel::getModelName(int index) const
{
    if (index >= 0 && index < (int)availableModels.size())
        return availableModels[index].name;
    return "Unknown";
}

void CustomModel::loadPlayerModel(Player &player, const char *modelPath, const char *texturePath)
{
    TraceLog(LOG_INFO, "Attempting to load model from: %s", modelPath);
    player.model = LoadModel(modelPath);

    if (player.model.meshCount > 0)
    {
        player.modelLoaded = true;
        TraceLog(LOG_INFO, "Player model loaded successfully with %d meshes", player.model.meshCount);

        // Load and apply texture if provided
        if (texturePath != nullptr)
        {
            Texture2D texture = LoadTexture(texturePath);
            if (texture.id > 0)
            {
                player.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
                TraceLog(LOG_INFO, "Texture applied to model successfully from: %s", texturePath);
            }
            else
            {
                TraceLog(LOG_WARNING, "Failed to load texture: %s", texturePath);
            }
        }
    }
    else
    {
        player.modelLoaded = false;
        TraceLog(LOG_WARNING, "Failed to load player model from %s, using fallback cube", modelPath);
    }
}

void CustomModel::drawPlayerModel(const Player &player)
{
    if (player.modelLoaded)
    {
        // Draw model at player's feet position (no offset)
        Vector3 modelPosition = player.position;

        // Apply rotation offsets (X, Y, Z) and player yaw
        // Need to use rlPushMatrix for proper 3D rotations
        rlPushMatrix();
        rlTranslatef(modelPosition.x, modelPosition.y, modelPosition.z);
        rlRotatef(player.modelRotationOffset.y + player.playerYaw, 0.0f, 1.0f, 0.0f); // Y rotation (yaw)
        rlRotatef(player.modelRotationOffset.x, 1.0f, 0.0f, 0.0f);                    // X rotation (pitch)
        rlRotatef(player.modelRotationOffset.z, 0.0f, 0.0f, 1.0f);                    // Z rotation (roll)
        rlScalef(player.modelScale.x, player.modelScale.y, player.modelScale.z);
        DrawModel(player.model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
        rlPopMatrix();
    }
    else
    {
        DrawCubeV(player.position, Vector3{1.0f, 2.0f, 1.0f}, GREEN);
        DrawCubeWiresV(player.position, Vector3{1.0f, 2.0f, 1.0f}, DARKGREEN);
    }
}