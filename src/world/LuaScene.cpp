#include "LuaScene.h"
#include "raylib.h"

LuaScene::LuaScene(const std::string &luaFilePath)
    : filePath(luaFilePath)
{
}

LuaScene::~LuaScene()
{
}

void LuaScene::Load()
{
    TraceLog(LOG_INFO, "LuaScene: Loading from '%s'...", filePath.c_str());

    // Load level data from Lua file
    levelData = SceneLoader::LoadFromFile(filePath);

    // Build scene object pool from loaded level data
    LoadObjects();

    // Create NPCs from level data
    LoadNPCs();

    TraceLog(LOG_INFO, "LuaScene: Loaded '%s'", levelData.name.c_str());
}

void LuaScene::Unload()
{
    TraceLog(LOG_INFO, "LuaScene: Unloading '%s'...", levelData.name.c_str());

    // Clear scene runtime data
    objects.clear();
    npcs.clear();

    TraceLog(LOG_INFO, "LuaScene: Unloaded");
}

void LuaScene::LoadObjects()
{
    objects = levelData.objects;
}

void LuaScene::LoadNPCs()
{
    for (const auto &npcData : levelData.npcs)
    {
        npcs.emplace_back(npcData.position, npcData.name, npcData.dialogueLines);
    }
}
