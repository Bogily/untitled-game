/**
 * @file ShaderUtil.h
 * @brief Utility functions for simplified shader parameter management
 */

#pragma once

#include "raylib.h"
#include <string>
#include <unordered_map>

/**
 * @brief Shader utility class for managing uniforms and textures
 *
 * Provides a interface for setting shader parameters without
 * repeatedly calling GetShaderLocation and SetShaderValue.
 */
class ShaderUtil
{
public:
    /**
     * @brief Construct shader utility for a specific shader
     * @param shader The shader to manage
     */
    explicit ShaderUtil(Shader shader);

    /**
     * @brief Set integer uniform
     * @param name Uniform name
     * @param value Integer value
     */
    void SetInt(const char *name, int value);

    /**
     * @brief Set float uniform
     * @param name Uniform name
     * @param value Float value
     */
    void SetFloat(const char *name, float value);

    /**
     * @brief Set Vector2 uniform
     * @param name Uniform name
     * @param value Vector2 value
     */
    void SetVec2(const char *name, Vector2 value);

    /**
     * @brief Set Vector3 uniform
     * @param name Uniform name
     * @param value Vector3 value
     */
    void SetVec3(const char *name, Vector3 value);

    /**
     * @brief Set Vector4 uniform
     * @param name Uniform name
     * @param value Vector4 value
     */
    void SetVec4(const char *name, Vector4 value);

    /**
     * @brief Set texture uniform
     * @param name Uniform name
     * @param texture Texture to bind
     */
    void SetTexture(const char *name, Texture2D texture);

    /**
     * @brief Get the underlying shader
     * @return Shader reference
     */
    Shader GetShader() const { return shader; }

    /**
     * @brief Clear cached uniform locations
     */
    void ClearCache();

private:
    Shader shader;                                      ///< The managed shader
    std::unordered_map<std::string, int> locationCache; ///< Cache of uniform locations

    /**
     * @brief Get or cache shader uniform location
     * @param name Uniform name
     * @return Shader location ID
     */
    int GetLocation(const char *name);
};

/**
 * @brief RAII wrapper for BeginShaderMode/EndShaderMode
 */
class ScopedShaderMode
{
public:
    explicit ScopedShaderMode(Shader shader) : active(true)
    {
        BeginShaderMode(shader);
    }

    ~ScopedShaderMode()
    {
        if (active)
            EndShaderMode();
    }

    // Delete copy operations
    ScopedShaderMode(const ScopedShaderMode &) = delete;
    ScopedShaderMode &operator=(const ScopedShaderMode &) = delete;

private:
    bool active;
};

/**
 * @brief RAII wrapper for BeginTextureMode/EndTextureMode
 */
class ScopedTextureMode
{
public:
    explicit ScopedTextureMode(RenderTexture2D target) : active(true)
    {
        BeginTextureMode(target);
    }

    ~ScopedTextureMode()
    {
        if (active)
            EndTextureMode();
    }

    // Delete copy operations
    ScopedTextureMode(const ScopedTextureMode &) = delete;
    ScopedTextureMode &operator=(const ScopedTextureMode &) = delete;

private:
    bool active;
};
