#include "SceneLoader.h"
#include "raylib.h"
#include <stdexcept>

SceneLoader::SceneLoader()
{
}

SceneLoader::~SceneLoader()
{
}

LevelData SceneLoader::LoadFromFile(const std::string &filepath)
{
    LevelData level;

    // Create Lua state
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    // Load and execute the Lua file
    if (luaL_dofile(L, filepath.c_str()) != LUA_OK)
    {
        const char *error = lua_tostring(L, -1);
        TraceLog(LOG_ERROR, "SceneLoader: Failed to load scene file '%s': %s", filepath.c_str(), error);
        lua_close(L);
        throw std::runtime_error("Failed to load scene file");
    }

    // The scene file should return a table on the stack
    if (!lua_istable(L, -1))
    {
        TraceLog(LOG_ERROR, "SceneLoader: Scene file '%s' must return a table", filepath.c_str());
        lua_close(L);
        throw std::runtime_error("Invalid scene file format");
    }

    // Read scene name
    lua_getfield(L, -1, "name");
    if (lua_isstring(L, -1))
    {
        level.name = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    // Read camera settings
    ReadCameraData(L, level);

    // Read player start position
    lua_getfield(L, -1, "playerStart");
    if (lua_istable(L, -1))
    {
        lua_rawgeti(L, -1, 1);
        lua_rawgeti(L, -2, 2);
        lua_rawgeti(L, -3, 3);
        level.playerStartPosition = {
            (float)lua_tonumber(L, -3),
            (float)lua_tonumber(L, -2),
            (float)lua_tonumber(L, -1)};
        lua_pop(L, 3);
    }
    lua_pop(L, 1);

    // Read objects
    ReadObjects(L, level);

    // Read NPCs
    ReadNPCs(L, level);

    // Read lights
    ReadLights(L, level);

    // Read particles
    ReadParticles(L, level);

    // Read grass
    ReadGrass(L, level);

    // Read water
    ReadWater(L, level);

    // Read skybox
    lua_getfield(L, -1, "skybox");
    if (lua_isstring(L, -1))
    {
        level.skyboxTexture = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    // Read gravity
    lua_getfield(L, -1, "gravity");
    if (lua_isnumber(L, -1))
    {
        level.gravity = (float)lua_tonumber(L, -1);
    }
    lua_pop(L, 1);

    lua_close(L);

    TraceLog(LOG_INFO, "SceneLoader: Loaded scene '%s' with %d objects, %d NPCs, %d lights",
             level.name.c_str(), level.objects.size(), level.npcs.size(), level.lights.size());

    return level;
}

Vector3 SceneLoader::ReadVector3(lua_State *L, const char *tableName)
{
    Vector3 result = {0.0f, 0.0f, 0.0f};

    lua_getfield(L, -1, tableName);
    if (lua_istable(L, -1))
    {
        lua_rawgeti(L, -1, 1);
        lua_rawgeti(L, -2, 2);
        lua_rawgeti(L, -3, 3);
        result = {
            (float)lua_tonumber(L, -3),
            (float)lua_tonumber(L, -2),
            (float)lua_tonumber(L, -1)};
        lua_pop(L, 3);
    }
    lua_pop(L, 1);

    return result;
}

Color SceneLoader::ReadColor(lua_State *L, const char *tableName)
{
    Color result = {255, 255, 255, 255};

    lua_getfield(L, -1, tableName);
    if (lua_istable(L, -1))
    {
        lua_rawgeti(L, -1, 1);
        lua_rawgeti(L, -2, 2);
        lua_rawgeti(L, -3, 3);
        lua_rawgeti(L, -4, 4);
        result = {
            (unsigned char)lua_tointeger(L, -4),
            (unsigned char)lua_tointeger(L, -3),
            (unsigned char)lua_tointeger(L, -2),
            (unsigned char)lua_tointeger(L, -1)};
        lua_pop(L, 4);
    }
    lua_pop(L, 1);

    return result;
}

void SceneLoader::ReadCameraData(lua_State *L, LevelData &level)
{
    lua_getfield(L, -1, "camera");
    if (lua_istable(L, -1))
    {
        level.camera.startPosition = ReadVector3(L, "position");
        level.camera.startTarget = ReadVector3(L, "target");

        lua_getfield(L, -1, "fov");
        if (lua_isnumber(L, -1))
            level.camera.startFOV = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "followDistance");
        if (lua_isnumber(L, -1))
            level.camera.followDistance = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "followHeight");
        if (lua_isnumber(L, -1))
            level.camera.followHeight = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "smoothness");
        if (lua_isnumber(L, -1))
            level.camera.smoothness = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

void SceneLoader::ReadObjects(lua_State *L, LevelData &level)
{
    lua_getfield(L, -1, "objects");
    if (lua_istable(L, -1))
    {
        int numObjects = luaL_len(L, -1);
        for (int i = 1; i <= numObjects; i++)
        {
            lua_rawgeti(L, -1, i);
            if (lua_istable(L, -1))
            {
                LevelData::ObjectData obj;

                lua_getfield(L, -1, "name");
                if (lua_isstring(L, -1))
                    obj.name = lua_tostring(L, -1);
                lua_pop(L, 1);

                obj.position = ReadVector3(L, "position");
                obj.rotation = ReadVector3(L, "rotation");
                obj.scale = ReadVector3(L, "scale");
                obj.albedo = ReadColor(L, "albedo");

                lua_getfield(L, -1, "modelType");
                if (lua_isstring(L, -1))
                    obj.modelType = lua_tostring(L, -1);
                lua_pop(L, 1);

                // Metallic is optional, defaults to -1.0 (use GLB value)
                lua_getfield(L, -1, "metallic");
                if (lua_isnumber(L, -1))
                    obj.metallic = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                // Roughness is optional, defaults to -1.0 (use GLB value)
                lua_getfield(L, -1, "roughness");
                if (lua_isnumber(L, -1))
                    obj.roughness = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                level.objects.push_back(obj);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
}

void SceneLoader::ReadNPCs(lua_State *L, LevelData &level)
{
    lua_getfield(L, -1, "npcs");
    if (lua_istable(L, -1))
    {
        int numNPCs = luaL_len(L, -1);
        for (int i = 1; i <= numNPCs; i++)
        {
            lua_rawgeti(L, -1, i);
            if (lua_istable(L, -1))
            {
                LevelData::NPCData npc;

                lua_getfield(L, -1, "name");
                if (lua_isstring(L, -1))
                    npc.name = lua_tostring(L, -1);
                lua_pop(L, 1);

                npc.position = ReadVector3(L, "position");

                lua_getfield(L, -1, "interactionRange");
                if (lua_isnumber(L, -1))
                    npc.interactionRange = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "dialogue");
                if (lua_istable(L, -1))
                {
                    int numLines = luaL_len(L, -1);
                    for (int j = 1; j <= numLines; j++)
                    {
                        lua_rawgeti(L, -1, j);
                        if (lua_isstring(L, -1))
                            npc.dialogueLines.push_back(lua_tostring(L, -1));
                        lua_pop(L, 1);
                    }
                }
                lua_pop(L, 1);

                level.npcs.push_back(npc);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
}

void SceneLoader::ReadLights(lua_State *L, LevelData &level)
{
    lua_getfield(L, -1, "lights");
    if (lua_istable(L, -1))
    {
        int numLights = luaL_len(L, -1);
        for (int i = 1; i <= numLights; i++)
        {
            lua_rawgeti(L, -1, i);
            if (lua_istable(L, -1))
            {
                LevelData::LightData light;

                lua_getfield(L, -1, "name");
                if (lua_isstring(L, -1))
                    light.name = lua_tostring(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "type");
                if (lua_isstring(L, -1))
                {
                    std::string typeStr = lua_tostring(L, -1);
                    light.type = (typeStr == "directional") ? 0 : 1;
                }
                lua_pop(L, 1);

                light.position = ReadVector3(L, "position");
                light.direction = ReadVector3(L, "direction");
                light.color = ReadColor(L, "color");

                lua_getfield(L, -1, "intensity");
                if (lua_isnumber(L, -1))
                    light.intensity = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "radius");
                if (lua_isnumber(L, -1))
                    light.radius = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                level.lights.push_back(light);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
}

void SceneLoader::ReadGrass(lua_State *L, LevelData &level)
{
    lua_getfield(L, -1, "grass");
    if (lua_istable(L, -1))
    {
        level.grassPosition = ReadVector3(L, "position");

        lua_getfield(L, -1, "width");
        if (lua_isnumber(L, -1))
            level.grassWidth = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "length");
        if (lua_isnumber(L, -1))
            level.grassLength = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "bladeCount");
        if (lua_isnumber(L, -1))
            level.grassBladeCount = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

void SceneLoader::ReadWater(lua_State *L, LevelData &level)
{
    lua_getfield(L, -1, "water");
    if (lua_istable(L, -1))
    {
        level.waterPosition = ReadVector3(L, "position");

        lua_getfield(L, -1, "width");
        if (lua_isnumber(L, -1))
            level.waterWidth = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "length");
        if (lua_isnumber(L, -1))
            level.waterLength = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

void SceneLoader::ReadParticles(lua_State *L, LevelData &level)
{
    lua_getfield(L, -1, "particles");
    if (lua_istable(L, -1))
    {
        int numEmitters = luaL_len(L, -1);
        for (int i = 1; i <= numEmitters; i++)
        {
            lua_rawgeti(L, -1, i);
            if (lua_istable(L, -1))
            {
                LevelData::ParticleEmitterData emitter;

                emitter.position = ReadVector3(L, "position");
                emitter.offset = ReadVector3(L, "offset");
                emitter.velocity = ReadVector3(L, "velocity");
                emitter.velocityRandom = ReadVector3(L, "velocityRandom");
                emitter.acceleration = ReadVector3(L, "acceleration");
                emitter.colorStart = ReadColor(L, "colorStart");
                emitter.colorEnd = ReadColor(L, "colorEnd");

                lua_getfield(L, -1, "sizeStart");
                if (lua_isnumber(L, -1))
                    emitter.sizeStart = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "sizeEnd");
                if (lua_isnumber(L, -1))
                    emitter.sizeEnd = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "sizeRandom");
                if (lua_isnumber(L, -1))
                    emitter.sizeRandom = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "lifeMin");
                if (lua_isnumber(L, -1))
                    emitter.lifeMin = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "lifeMax");
                if (lua_isnumber(L, -1))
                    emitter.lifeMax = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "emissionRate");
                if (lua_isnumber(L, -1))
                    emitter.emissionRate = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "maxParticles");
                if (lua_isnumber(L, -1))
                    emitter.maxParticles = (int)lua_tointeger(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "blendMode");
                if (lua_isstring(L, -1))
                    emitter.blendMode = lua_tostring(L, -1);
                else
                    emitter.blendMode = "alpha"; // Default
                lua_pop(L, 1);

                // Support legacy boolean 'blending' for backward compatibility
                lua_getfield(L, -1, "blending");
                if (lua_isboolean(L, -1))
                {
                    bool add = lua_toboolean(L, -1);
                    if (add && emitter.blendMode == "alpha")
                        emitter.blendMode = "add";
                }
                lua_pop(L, 1);

                lua_getfield(L, -1, "textureName");
                if (lua_isstring(L, -1))
                    emitter.textureName = lua_tostring(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "texture"); // This was texturePath
                if (lua_isstring(L, -1))
                    emitter.texturePath = lua_tostring(L, -1);
                lua_pop(L, 1);

                level.particles.push_back(emitter);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
}
