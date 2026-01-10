#pragma once
#include "../world/Scene.h"
#include "../scripting/SceneLoader.h"
#include <string>

// Scene loaded from a Lua file
class LuaScene : public Scene
{
public:
    LuaScene(const std::string &luaFilePath);
    ~LuaScene() override;

    void Load() override;
    void Unload() override;

    LevelData &GetLevelData() override { return levelData; }
    std::vector<NPC> &GetNPCs() override { return npcs; }

private:
    std::string filePath;
    LevelData levelData;
    std::vector<NPC> npcs;

    void LoadNPCs();
};
