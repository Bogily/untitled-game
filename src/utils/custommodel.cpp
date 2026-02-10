#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "../actors/Player.h"
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
    RenderComponent &render = player.GetRender();
    if (render.modelLoaded)
    {
        UnloadModel(render.model);
        render.modelLoaded = false;
    }

    // Load new model
    const char *texPath = modelData.texturePath.empty() ? nullptr : modelData.texturePath.c_str();
    loadPlayerModel(player, modelData.modelPath.c_str(), texPath);

    // Set model scale and rotation
    render.modelScale = modelData.scale;
    render.modelRotationOffset = modelData.rotationOffset;

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
    RenderComponent &render = player.GetRender();
    render.model = LoadModel(modelPath);

    if (render.model.meshCount > 0)
    {
        render.modelLoaded = true;
        TraceLog(LOG_INFO, "Player model loaded successfully with %d meshes", render.model.meshCount);

        // Load and apply texture if provided
        if (texturePath != nullptr)
        {
            Texture2D texture = LoadTexture(texturePath);
            if (texture.id > 0)
            {
                render.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
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
        render.modelLoaded = false;
        TraceLog(LOG_WARNING, "Failed to load player model from %s, using fallback cube", modelPath);
    }
}

void CustomModel::drawPlayerModel(const Player &player)
{
    const TransformComponent &transform = player.GetTransform();
    const RenderComponent &render = player.GetRender();
    if (render.modelLoaded)
    {
        // Draw model at player's feet position (no offset)
        Vector3 modelPosition = transform.position;

        // Apply rotation offsets (X, Y, Z) and player yaw
        // Need to use rlPushMatrix for proper 3D rotations
        rlPushMatrix();
        rlTranslatef(modelPosition.x, modelPosition.y, modelPosition.z);
        rlRotatef(render.modelRotationOffset.y + player.playerYaw, 0.0f, 1.0f, 0.0f); // Y rotation (yaw)
        rlRotatef(render.modelRotationOffset.x, 1.0f, 0.0f, 0.0f);                    // X rotation (pitch)
        rlRotatef(render.modelRotationOffset.z, 0.0f, 0.0f, 1.0f);                    // Z rotation (roll)
        rlScalef(render.modelScale.x, render.modelScale.y, render.modelScale.z);
        DrawModel(render.model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
        rlPopMatrix();
    }
    else
    {
        DrawCubeV(transform.position, Vector3{1.0f, 2.0f, 1.0f}, GREEN);
        DrawCubeWiresV(transform.position, Vector3{1.0f, 2.0f, 1.0f}, DARKGREEN);
    }
}