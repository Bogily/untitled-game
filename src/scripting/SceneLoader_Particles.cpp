#include "SceneLoader.h"

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
                if (lua_isnumber(L, -1)) emitter.sizeStart = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "sizeEnd");
                if (lua_isnumber(L, -1)) emitter.sizeEnd = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "sizeRandom");
                if (lua_isnumber(L, -1)) emitter.sizeRandom = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "lifeMin");
                if (lua_isnumber(L, -1)) emitter.lifeMin = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "lifeMax");
                if (lua_isnumber(L, -1)) emitter.lifeMax = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "emissionRate");
                if (lua_isnumber(L, -1)) emitter.emissionRate = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "maxParticles");
                if (lua_isnumber(L, -1)) emitter.maxParticles = (int)lua_tointeger(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "blendMode");
                if (lua_isstring(L, -1)) emitter.blendMode = lua_tostring(L, -1);
                else emitter.blendMode = "alpha"; // Default
                lua_pop(L, 1);

                // Support legacy boolean 'blending' for backward compatibility
                lua_getfield(L, -1, "blending");
                if (lua_isboolean(L, -1)) {
                    bool add = lua_toboolean(L, -1);
                    if (add && emitter.blendMode == "alpha") emitter.blendMode = "add";
                }
                lua_pop(L, 1);

                lua_getfield(L, -1, "textureName");
                if (lua_isstring(L, -1)) emitter.textureName = lua_tostring(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "texture"); // This was texturePath
                if (lua_isstring(L, -1)) emitter.texturePath = lua_tostring(L, -1);
                lua_pop(L, 1);

                level.particles.push_back(emitter);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
}
