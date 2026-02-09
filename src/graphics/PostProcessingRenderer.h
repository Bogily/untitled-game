/**
 * @file PostProcessingRenderer.h
 * @brief Post-processing effects pipeline
 */

#pragma once

#include "raylib.h"
#include "raymath.h"

/**
 * @brief Post-processing renderer applying fullscreen effects
 *
 * Captures scene to texture and applies effects like grayscale
 * and depth buffer visualization. Supports multiple effect passes.
 */
class PostProcessingRenderer
{
public:
    /**
     * @brief Construct post-processing renderer
     */
    PostProcessingRenderer();

    /**
     * @brief Destroy renderer and cleanup resources
     */
    ~PostProcessingRenderer();

    /**
     * @brief Initialize render textures and shaders
     * @param screenWidth Screen width in pixels
     * @param screenHeight Screen height in pixels
     */
    void Init(int screenWidth, int screenHeight);

    /**
     * @brief Shutdown and cleanup resources
     */
    void Shutdown();

    /**
     * @brief Begin rendering to offscreen texture
     *
     * Call before rendering scene geometry.
     */
    void BeginSceneCapture();

    /**
     * @brief End scene capture
     *
     * Call after all scene geometry is rendered.
     */
    void EndSceneCapture();

    /**
     * @brief Apply configured post-processing effects
     *
     * Renders processed scene to screen.
     */
    void ApplyEffects();

    /**
     * @brief Enable or disable grayscale effect
     * @param enable True to enable grayscale
     */
    void SetGrayscaleEnabled(bool enable) { enableGrayscale = enable; }

    /**
     * @brief Check if grayscale is enabled
     * @return True if enabled
     */
    bool GetGrayscaleEnabled() const { return enableGrayscale; }

    /**
     * @brief Enable or disable depth buffer visualization
     * @param enable True to show depth buffer
     */
    void SetDepthDebugEnabled(bool enable) { enableDepthDebug = enable; }

    /**
     * @brief Check if depth debug is enabled
     * @return True if enabled
     */
    bool GetDepthDebugEnabled() const { return enableDepthDebug; }

    /**
     * @brief Get scene render texture
     * @return Scene texture with depth attachment
     */
    RenderTexture2D GetSceneTexture() const { return sceneTexture; }

private:
    RenderTexture2D sceneTexture; ///< Main scene render target with depth attachment

    Shader grayscaleShader; ///< Grayscale effect shader
    Shader depthShader;     ///< Depth visualization shader

    int width;  ///< Screen width
    int height; ///< Screen height

    bool enableGrayscale;  ///< Grayscale effect toggle
    bool enableDepthDebug; ///< Depth debug toggle

    /**
     * @brief Render fullscreen quad with shader
     * @param shader Shader to apply
     * @param source Source texture
     */
    void RenderFullscreenQuad(Shader shader, RenderTexture2D source);

    /**
     * @brief Create render texture with depth attachment
     * @param screenWidth Width in pixels
     * @param screenHeight Height in pixels
     * @return Render texture with depth buffer
     */
    RenderTexture2D LoadRenderTextureWithDepth(int screenWidth, int screenHeight);

    /**
     * @brief Unload render texture with depth buffer
     * @param target Render texture to unload
     */
    void UnloadRenderTextureWithDepth(RenderTexture2D target);
};
