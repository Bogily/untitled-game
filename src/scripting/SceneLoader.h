#pragma once
#include "../world/Level.h"
#include <string>

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

// Loads scenes from Lua files
class SceneLoader
{
public:
    SceneLoader();
    ~SceneLoader();

    // Load a scene from a Lua file
    static LevelData LoadFromFile(const std::string &filepath);

private:
    // Helper functions to parse Lua tables
    static Vector3 ReadVector3(lua_State *L, const char *tableName);
    static Color ReadColor(lua_State *L, const char *tableName);
    static void ReadCameraData(lua_State *L, LevelData &level);
    static void ReadObjects(lua_State *L, LevelData &level);
    static void ReadNPCs(lua_State *L, LevelData &level);
    static void ReadLights(lua_State *L, LevelData &level);
    static void ReadGrass(lua_State *L, LevelData &level);
    static void ReadWater(lua_State *L, LevelData &level);
};
