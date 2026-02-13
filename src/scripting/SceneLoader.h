/**
 * @file SceneLoader.h
 * @brief Lua-based scene file loader
 */

#pragma once
#include "../world/Level.h"
#include <string>

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

/**
 * @brief Scene loader for Lua-based scene files
 *
 * Parses Lua scene files containing:
 * - Camera configuration
 * - Objects (models, transforms, optional static/dynamic mobility)
 * - NPCs (characters, dialogue)
 * - Lights (dynamic lighting)
 * - Particles (effects)
 * - Grass (vegetation)
 * - Water (bodies of water)
 */
class SceneLoader
{
public:
    /**
     * @brief Construct scene loader
     */
    SceneLoader();

    /**
     * @brief Destroy scene loader
     */
    ~SceneLoader();

    /**
     * @brief Load scene from Lua file
     * @param filepath Path to .lua scene file
     * @return Loaded level data
     */
    static LevelData LoadFromFile(const std::string &filepath);

private:
    /**
     * @brief Parse Vector3 from Lua table
     * @param L Lua state
     * @param tableName Table name to read
     * @return Parsed vector
     */
    static Vector3 ReadVector3(lua_State *L, const char *tableName);

    /**
     * @brief Parse Color from Lua table
     * @param L Lua state
     * @param tableName Table name to read
     * @return Parsed color
     */
    static Color ReadColor(lua_State *L, const char *tableName);

    /**
     * @brief Parse camera data from Lua
     * @param L Lua state
     * @param level Level data to populate
     */
    static void ReadCameraData(lua_State *L, LevelData &level);

    /**
     * @brief Parse object data from Lua
     * @param L Lua state
     * @param level Level data to populate
     */
    static void ReadObjects(lua_State *L, LevelData &level);

    /**
     * @brief Parse NPC data from Lua
     * @param L Lua state
     * @param level Level data to populate
     */
    static void ReadNPCs(lua_State *L, LevelData &level);

    /**
     * @brief Parse light data from Lua
     * @param L Lua state
     * @param level Level data to populate
     */
    static void ReadLights(lua_State *L, LevelData &level);

    /**
     * @brief Parse particle data from Lua
     * @param L Lua state
     * @param level Level data to populate
     */
    static void ReadParticles(lua_State *L, LevelData &level);

    /**
     * @brief Parse grass data from Lua
     * @param L Lua state
     * @param level Level data to populate
     */
    static void ReadGrass(lua_State *L, LevelData &level);

    /**
     * @brief Parse water data from Lua
     * @param L Lua state
     * @param level Level data to populate
     */
    static void ReadWater(lua_State *L, LevelData &level);
};
