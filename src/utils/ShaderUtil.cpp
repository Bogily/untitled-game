#include "ShaderUtil.h"

ShaderUtil::ShaderUtil(Shader shader)
    : shader(shader)
{
}

void ShaderUtil::SetInt(const char *name, int value)
{
    int loc = GetLocation(name);
    if (loc >= 0)
        SetShaderValue(shader, loc, &value, SHADER_UNIFORM_INT);
}

void ShaderUtil::SetFloat(const char *name, float value)
{
    int loc = GetLocation(name);
    if (loc >= 0)
        SetShaderValue(shader, loc, &value, SHADER_UNIFORM_FLOAT);
}

void ShaderUtil::SetVec2(const char *name, Vector2 value)
{
    int loc = GetLocation(name);
    if (loc >= 0)
        SetShaderValue(shader, loc, &value, SHADER_UNIFORM_VEC2);
}

void ShaderUtil::SetVec3(const char *name, Vector3 value)
{
    int loc = GetLocation(name);
    if (loc >= 0)
        SetShaderValue(shader, loc, &value, SHADER_UNIFORM_VEC3);
}

void ShaderUtil::SetVec4(const char *name, Vector4 value)
{
    int loc = GetLocation(name);
    if (loc >= 0)
        SetShaderValue(shader, loc, &value, SHADER_UNIFORM_VEC4);
}

void ShaderUtil::SetTexture(const char *name, Texture2D texture)
{
    int loc = GetLocation(name);
    if (loc >= 0)
        SetShaderValueTexture(shader, loc, texture);
}

void ShaderUtil::ClearCache()
{
    locationCache.clear();
}

int ShaderUtil::GetLocation(const char *name)
{
    auto it = locationCache.find(name);
    if (it != locationCache.end())
        return it->second;

    int loc = GetShaderLocation(shader, name);
    locationCache[name] = loc;
    return loc;
}
