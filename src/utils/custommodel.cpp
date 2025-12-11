#include "raylib.h"
#include "raymath.h"
#include "../player/Player.h"
#include "custommodel.h"

void CustomModel::loadPlayerModel(Player& player, const char* modelPath, const char* texturePath)
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


void CustomModel::drawPlayerModel(const Player& player)
{
    if (player.modelLoaded)
    {
        Vector3 rotationAxis = {0.0f, 1.0f, 0.0f};
        Vector3 modelPosition = {player.position.x, player.position.y - 1.0f, player.position.z};
        DrawModelEx(player.model, modelPosition, rotationAxis, player.playerYaw, 
                   (Vector3){0.04f, 0.04f, 0.04f}, WHITE);
    }
    else
    {
        DrawCubeV(player.position, (Vector3){1.0f, 2.0f, 1.0f}, GREEN);
        DrawCubeWiresV(player.position, (Vector3){1.0f, 2.0f, 1.0f}, DARKGREEN);
    }
}